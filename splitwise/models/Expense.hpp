#ifndef SPLITWISE_EXPENSE_HPP
#define SPLITWISE_EXPENSE_HPP

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../money/Money.hpp"
#include "Ids.hpp"
#include "Split.hpp"

// One recorded expense: who paid, how much, and what each participant owes.
//
// The constructor is the only place that can produce an Expense, and it refuses
// every state the rest of the program would otherwise have to check for:
//
//   - the shares add up to the total, exactly, in paise
//   - nobody appears twice
//   - the total is positive
//
// The original checked none of it. `EqualSplit` divided a double by the number
// of participants, so the shares rarely summed back to the total, and whatever
// float came out became the recorded debt.
class Expense {
public:
    Expense(ExpenseId id, std::string description, Money::Amount total, UserId paidBy,
            std::vector<Split> splits, GroupId groupId)
        : id_(std::move(id)),
          description_(std::move(description)),
          total_(total),
          paidBy_(std::move(paidBy)),
          splits_(std::move(splits)),
          groupId_(std::move(groupId)) {
        if (id_.empty()) {
            throw AppError("expense: id must not be empty");
        }
        if (description_.empty()) {
            throw AppError("expense " + id_ + ": description must not be empty");
        }
        if (total_ <= 0) {
            throw AppError("expense " + id_ + ": total must be positive, got " +
                           Money::format(total_));
        }
        if (paidBy_.empty()) {
            throw AppError("expense " + id_ + ": payer must not be empty");
        }
        if (splits_.empty()) {
            throw AppError("expense " + id_ + ": at least one share is required");
        }

        std::set<UserId> seen;
        Money::Amount stated = 0;
        for (const Split &split : splits_) {
            if (!seen.insert(split.participant()).second) {
                throw AppError("expense " + id_ + ": " + split.participant() +
                               " appears twice");
            }
            stated += split.amount();
        }

        if (stated != total_) {
            throw AppError("expense " + id_ + ": shares add up to " + Money::format(stated) +
                           " but the expense is " + Money::format(total_));
        }
    }

    const ExpenseId &id() const {
        return id_;
    }

    const std::string &description() const {
        return description_;
    }

    Money::Amount total() const {
        return total_;
    }

    const UserId &paidBy() const {
        return paidBy_;
    }

    const std::vector<Split> &splits() const {
        return splits_;
    }

    // Empty for a personal (non-group) expense.
    const GroupId &groupId() const {
        return groupId_;
    }

    bool isPersonal() const {
        return groupId_.empty();
    }

    // Zero when the user has no share. A zero-sized share is indistinguishable
    // from this, and that is fine: neither creates a debt.
    Money::Amount shareOf(const UserId &participant) const {
        for (const Split &split : splits_) {
            if (split.participant() == participant) {
                return split.amount();
            }
        }
        return 0;
    }

private:
    ExpenseId id_;
    std::string description_;
    Money::Amount total_;
    UserId paidBy_;
    std::vector<Split> splits_;
    GroupId groupId_;
};

#endif // SPLITWISE_EXPENSE_HPP
