#ifndef SPLITWISE_GROUP_HPP
#define SPLITWISE_GROUP_HPP

#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../money/Money.hpp"
#include "../observers/GroupEvent.hpp"
#include "../observers/GroupListener.hpp"
#include "../settlement/DebtSimplifier.hpp"
#include "Expense.hpp"
#include "ExpenseDraft.hpp"
#include "Ids.hpp"
#include "Ledger.hpp"

// One group: its members, its expense book, and the debts between them.
//
// The group knows ids, never User objects. The original stored `vector<User*>`
// members, so a group outlived nothing safely and every balance lookup had to go
// back through the directory and dereference a pointer that could be null.
//
// Expense ids are scoped to the group (`group1-e2`), so an id is meaningful on
// its own and two groups can never collide on one.
class Group {
public:
    Group(GroupId id, std::string name) : id_(std::move(id)), name_(std::move(name)) {
        if (id_.empty()) {
            throw AppError("group: id must not be empty");
        }
        if (name_.empty()) {
            throw AppError("group " + id_ + ": name must not be empty");
        }
    }

    const GroupId &id() const {
        return id_;
    }

    const std::string &name() const {
        return name_;
    }

    const std::set<UserId> &members() const {
        return members_;
    }

    bool isMember(const UserId &userId) const {
        return members_.find(userId) != members_.end();
    }

    void addMember(const UserId &userId) {
        if (userId.empty()) {
            throw AppError("group " + id_ + ": member id must not be empty");
        }
        if (!members_.insert(userId).second) {
            throw AppError("group " + id_ + ": " + userId + " is already a member");
        }
    }

    // Refused while the user is part of any outstanding obligation, in either
    // direction. Removing them anyway would leave the ledger pointing at someone
    // the group no longer contains, and the remaining members' balances would
    // quietly stop summing to zero.
    //
    // Already-recorded expenses keep referencing the user: they are history, and
    // reshaping history to match the current member list is how books stop
    // matching reality.
    void removeMember(const UserId &userId) {
        if (!isMember(userId)) {
            throw AppError("group " + id_ + ": " + userId + " is not a member");
        }
        if (ledger_.hasDebtsWith(userId)) {
            throw AppError("group " + id_ + ": cannot remove " + userId + " - " +
                           describeOpenBalance(userId));
        }

        members_.erase(userId);
    }

    const Expense &addExpense(const ExpenseDraft &draft) {
        if (!isMember(draft.paidBy)) {
            throw AppError("group " + id_ + ": payer " + draft.paidBy + " is not a member");
        }
        for (const UserId &participant : draft.participants) {
            if (!isMember(participant)) {
                throw AppError("group " + id_ + ": " + participant +
                               " is not a member and cannot share an expense");
            }
        }

        // The sequence only advances on success, so a refused draft does not
        // burn an id.
        const ExpenseId candidateId = id_ + "-e" + std::to_string(nextExpenseSequence_);
        const Expense expense = draft.build(candidateId, id_);
        ++nextExpenseSequence_;

        ledger_.record(expense);
        const Expense &stored = expenses_.emplace(expense.id(), std::move(expense)).first->second;

        publish(GroupEvent::Kind::ExpenseAdded, stored.paidBy(), "", stored.description(),
                stored.total());
        return stored;
    }

    // Records a payment between two members. More than is owed is allowed: the
    // excess becomes a credit in the other direction, which is what the money
    // actually did.
    void settle(const UserId &from, const UserId &to, Money::Amount amount) {
        if (!isMember(from)) {
            throw AppError("group " + id_ + ": " + from + " is not a member");
        }
        if (!isMember(to)) {
            throw AppError("group " + id_ + ": " + to + " is not a member");
        }
        if (from == to) {
            throw AppError("group " + id_ + ": " + from + " cannot settle with themselves");
        }
        if (amount <= 0) {
            throw AppError("group " + id_ + ": settlement must be positive, got " +
                           Money::format(amount));
        }

        ledger_.addDebt(from, to, -amount);
        publish(GroupEvent::Kind::Settlement, from, to, "", amount);
    }

    // Rewrites the ledger so each person only deals with the people they are
    // actually up or down against. Returns the transaction count afterwards.
    std::size_t simplifyDebts() {
        const std::size_t before = ledger_.transactionCount();
        ledger_.replaceWith(DebtSimplifier::simplify(ledger_));
        const std::size_t after = ledger_.transactionCount();

        publish(GroupEvent::Kind::DebtsSimplified, "", "",
                std::to_string(before) + " transactions -> " + std::to_string(after), 0);
        return after;
    }

    const Ledger &ledger() const {
        return ledger_;
    }

    const std::map<ExpenseId, Expense> &expenses() const {
        return expenses_;
    }

    const Expense &expense(const ExpenseId &expenseId) const {
        const auto found = expenses_.find(expenseId);
        if (found == expenses_.end()) {
            throw AppError("group " + id_ + ": unknown expense " + expenseId);
        }
        return found->second;
    }

    std::size_t expenseCount() const {
        return expenses_.size();
    }

    // Borrowed, not owned. The listener has to outlive its subscription.
    void subscribe(GroupListener *listener) {
        if (listener == nullptr) {
            throw AppError("group " + id_ + ": listener must not be null");
        }
        if (std::find(listeners_.begin(), listeners_.end(), listener) != listeners_.end()) {
            throw AppError("group " + id_ + ": listener is already subscribed");
        }
        listeners_.push_back(listener);
    }

    void unsubscribe(GroupListener *listener) {
        const auto found = std::find(listeners_.begin(), listeners_.end(), listener);
        if (found == listeners_.end()) {
            throw AppError("group " + id_ + ": listener is not subscribed");
        }
        listeners_.erase(found);
    }

    std::size_t listenerCount() const {
        return listeners_.size();
    }

private:
    std::string describeOpenBalance(const UserId &userId) const {
        const Money::Amount net = ledger_.netOf(userId);
        if (net < 0) {
            return "they still owe " + Money::format(-net);
        }
        if (net > 0) {
            return "they are still owed " + Money::format(net);
        }
        return "they still have unsettled debts";
    }

    void publish(GroupEvent::Kind kind, UserId actor, UserId counterparty,
                 std::string description, Money::Amount amount) {
        GroupEvent event;
        event.kind = kind;
        event.groupId = id_;
        event.groupName = name_;
        event.actor = std::move(actor);
        event.counterparty = std::move(counterparty);
        event.description = std::move(description);
        event.amount = amount;
        event.recipients.assign(members_.begin(), members_.end());

        // A copy, so a listener may subscribe or unsubscribe while handling an
        // event without invalidating the iteration. A listener removed during a
        // dispatch still receives that dispatch.
        const std::vector<GroupListener *> snapshot = listeners_;
        for (GroupListener *listener : snapshot) {
            listener->onGroupEvent(event);
        }
    }

    GroupId id_;
    std::string name_;
    std::set<UserId> members_;
    Ledger ledger_;
    std::map<ExpenseId, Expense> expenses_;
    std::vector<GroupListener *> listeners_;
    std::size_t nextExpenseSequence_ = 1;
};

#endif // SPLITWISE_GROUP_HPP
