#ifndef SPLITWISE_SPLITWISE_HPP
#define SPLITWISE_SPLITWISE_HPP

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../models/Expense.hpp"
#include "../models/ExpenseDraft.hpp"
#include "../models/Group.hpp"
#include "../models/Ids.hpp"
#include "../models/Ledger.hpp"
#include "../models/User.hpp"
#include "../money/Money.hpp"
#include "../observers/ConsoleNotifier.hpp"
#include "UserDirectory.hpp"

// FACADE. Users, groups, personal expenses and notifications behind one object.
//
// The original was also a Singleton, and the singleton was the problem: a
// static `Splitwise*` filled by `new`, never freed, unsynchronised, and
// impossible to reset - so one demo run leaves state that the next one inherits.
// Ownership is explicit here and the caller decides where the app lives.
//
// Every group created gets the one notifier subscribed to it, so the wiring
// happens once instead of at each call site.
class Splitwise {
public:
    Splitwise() : notifier_(directory_) {}

    Splitwise(const Splitwise &) = delete;
    Splitwise &operator=(const Splitwise &) = delete;

    // -------------------------------------------------------------- users
    const User &createUser(std::string name, std::string email) {
        return directory_.add(std::move(name), std::move(email));
    }

    const User &user(const UserId &userId) const {
        return directory_.get(userId);
    }

    const UserDirectory &directory() const {
        return directory_;
    }

    // ------------------------------------------------------------- groups
    const Group &createGroup(std::string name) {
        const GroupId groupId = "group" + std::to_string(nextGroupId_++);
        Group &group = groups_.emplace(groupId, Group(groupId, std::move(name))).first->second;
        group.subscribe(&notifier_);
        return group;
    }

    const Group &group(const GroupId &groupId) const {
        const auto found = groups_.find(groupId);
        if (found == groups_.end()) {
            throw AppError("unknown group " + groupId);
        }
        return found->second;
    }

    Group &group(const GroupId &groupId) {
        const auto found = groups_.find(groupId);
        if (found == groups_.end()) {
            throw AppError("unknown group " + groupId);
        }
        return found->second;
    }

    const std::map<GroupId, Group> &groups() const {
        return groups_;
    }

    std::size_t groupCount() const {
        return groups_.size();
    }

    // The directory is checked first so an unknown id says so, instead of
    // "not a member" for someone who does not exist at all.
    void addMember(const GroupId &groupId, const UserId &userId) {
        directory_.get(userId);
        group(groupId).addMember(userId);
    }

    void removeMember(const GroupId &groupId, const UserId &userId) {
        directory_.get(userId);
        group(groupId).removeMember(userId);
    }

    // ------------------------------------------------- group money flows
    const Expense &addGroupExpense(const GroupId &groupId, const ExpenseDraft &draft) {
        requireKnown(draft);
        return group(groupId).addExpense(draft);
    }

    void settleInGroup(const GroupId &groupId, const UserId &from, const UserId &to,
                       Money::Amount amount) {
        group(groupId).settle(from, to, amount);
    }

    std::size_t simplifyDebts(const GroupId &groupId) {
        return group(groupId).simplifyDebts();
    }

    // -------------------------------------------------- personal expenses
    const Expense &addPersonalExpense(const ExpenseDraft &draft) {
        requireKnown(draft);

        const ExpenseId candidateId = "personal-e" + std::to_string(nextPersonalExpense_);
        const Expense expense = draft.build(candidateId, "");
        ++nextPersonalExpense_;

        personalLedger_.record(expense);
        return personalExpenses_.emplace(expense.id(), std::move(expense)).first->second;
    }

    void settlePersonally(const UserId &from, const UserId &to, Money::Amount amount) {
        directory_.get(from);
        directory_.get(to);
        if (from == to) {
            throw AppError(from + " cannot settle with themselves");
        }
        if (amount <= 0) {
            throw AppError("personal settlement must be positive, got " + Money::format(amount));
        }
        personalLedger_.addDebt(from, to, -amount);
    }

    const Ledger &personalLedger() const {
        return personalLedger_;
    }

    const std::map<ExpenseId, Expense> &personalExpenses() const {
        return personalExpenses_;
    }

    // ---------------------------------------------------------- balances
    // Positive: the user is owed this overall. Negative: the user owes.
    Money::Amount netBalanceOf(const UserId &userId) const {
        directory_.get(userId);

        Money::Amount net = personalLedger_.netOf(userId);
        for (const auto &entry : groups_) {
            if (!entry.second.isMember(userId)) {
                continue;
            }
            net += entry.second.ledger().netOf(userId);
        }
        return net;
    }

    // Per counterparty, across every group and the personal book. Positive means
    // that counterparty owes the user.
    std::map<UserId, Money::Amount> obligationsOf(const UserId &userId) const {
        directory_.get(userId);

        std::map<UserId, Money::Amount> merged;
        const auto addAll = [&merged](const std::map<UserId, Money::Amount> &single) {
            for (const auto &entry : single) {
                merged[entry.first] += entry.second;
            }
        };

        addAll(personalLedger_.obligationsOf(userId));
        for (const auto &entry : groups_) {
            if (!entry.second.isMember(userId)) {
                continue;
            }
            addAll(entry.second.ledger().obligationsOf(userId));
        }

        // A pair that nets to zero in two groups owes nothing overall, so it is
        // not an obligation and must not be printed as one.
        for (auto it = merged.begin(); it != merged.end();) {
            if (it->second == 0) {
                it = merged.erase(it);
            } else {
                ++it;
            }
        }
        return merged;
    }

private:
    void requireKnown(const ExpenseDraft &draft) const {
        directory_.get(draft.paidBy);
        for (const UserId &participant : draft.participants) {
            directory_.get(participant);
        }
    }

    // Declared before the notifier: the notifier borrows a reference to it.
    UserDirectory directory_;
    ConsoleNotifier notifier_;

    std::size_t nextGroupId_ = 1;
    std::map<GroupId, Group> groups_;

    std::size_t nextPersonalExpense_ = 1;
    Ledger personalLedger_;
    std::map<ExpenseId, Expense> personalExpenses_;
};
#endif // SPLITWISE_SPLITWISE_HPP
