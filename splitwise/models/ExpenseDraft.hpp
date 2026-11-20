#ifndef SPLITWISE_EXPENSE_DRAFT_HPP
#define SPLITWISE_EXPENSE_DRAFT_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../splits/SplitFactory.hpp"
#include "../splits/SplitSpec.hpp"
#include "../splits/SplitStrategy.hpp"
#include "Expense.hpp"
#include "Ids.hpp"

// What a caller supplies to record an expense: no id, no shares, no bookkeeping.
//
// Everything an Expense needs beyond this is derived - the split strategy turns
// the participants and the spec into shares, and the owner (group or the
// personal book) supplies the id. Keeping the draft free of those means a
// caller cannot hand in shares that disagree with the split it asked for.
struct ExpenseDraft {
    std::string description;
    Money::Amount amount = 0;
    UserId paidBy;
    std::vector<UserId> participants;
    SplitSpec split = SplitSpec::equal();

    // The single place where a draft becomes an Expense, shared by group and
    // personal expenses so both obey the same arithmetic.
    //
    // Members are checked by the caller, not here: a draft only knows about
    // people who exist, not about who belongs to which group.
    Expense build(const ExpenseId &id, const GroupId &groupId) const {
        if (description.empty()) {
            throw AppError("expense: description must not be empty");
        }
        if (paidBy.empty()) {
            throw AppError("expense: payer must not be empty");
        }

        const std::unique_ptr<SplitStrategy> strategy = SplitFactory::create(split);
        return Expense(id, description, amount, paidBy, strategy->split(amount, participants),
                       groupId);
    }

    static ExpenseDraft equal(std::string description, Money::Amount amount, UserId paidBy,
                              std::vector<UserId> participants) {
        return ExpenseDraft{std::move(description), amount, std::move(paidBy),
                            std::move(participants), SplitSpec::equal()};
    }

    static ExpenseDraft exact(std::string description, Money::Amount amount, UserId paidBy,
                              std::vector<UserId> participants, std::vector<double> rupees) {
        return ExpenseDraft{std::move(description), amount, std::move(paidBy),
                            std::move(participants), SplitSpec::exact(std::move(rupees))};
    }

    static ExpenseDraft percentage(std::string description, Money::Amount amount, UserId paidBy,
                                   std::vector<UserId> participants,
                                   std::vector<double> percentages) {
        return ExpenseDraft{std::move(description), amount, std::move(paidBy),
                            std::move(participants),
                            SplitSpec::percentage(std::move(percentages))};
    }
};

#endif // SPLITWISE_EXPENSE_DRAFT_HPP
