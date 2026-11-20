#ifndef SPLITWISE_LEDGER_HPP
#define SPLITWISE_LEDGER_HPP

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../money/Money.hpp"
#include "Expense.hpp"
#include "Ids.hpp"

// One recorded obligation.
struct Debt {
    UserId debtor;
    UserId creditor;
    Money::Amount amount;  // always positive
};

// Who owes whom, for one group or for personal (non-group) expenses.
//
// One entry per pair, never two. The original kept a symmetric matrix:
//
//     groupBalances[A][B] = +200;   // B owes A
//     groupBalances[B][A] = -200;   // A owes B -200
//
// Two rows described one debt, so every write had to update both, every read had
// to pick a sign convention, and `updateGroupBalance` had to re-check both sides
// for zero. Worse, `DebtSimplifier` then had to be careful to count each pair
// once ("only process positive amounts to avoid double counting").
//
// Here the pair is normalised: the ids are ordered, the smaller one first, and
// the stored value is what the FIRST owes the SECOND. Negative means the second
// owes the first, so a pair that cancels out becomes zero and is erased. There is
// exactly one number per relationship and no invariant to maintain by hand.
class Ledger {
public:
    // Records that `debtor` owes `creditor` `amount`. A negative amount reverses
    // the direction, which is how a settlement is applied.
    void addDebt(const UserId &debtor, const UserId &creditor, Money::Amount amount) {
        if (debtor.empty() || creditor.empty()) {
            throw AppError("ledger: both ids are required");
        }
        if (debtor == creditor) {
            throw AppError("ledger: " + debtor + " cannot owe themselves");
        }
        if (amount == 0) {
            return;
        }

        const bool debtorIsFirst = debtor < creditor;
        const UserId &first = debtorIsFirst ? debtor : creditor;
        const UserId &second = debtorIsFirst ? creditor : debtor;

        const Money::Amount updated =
            stored(first, second) + (debtorIsFirst ? amount : -amount);

        if (updated == 0) {
            forget(first, second);
            return;
        }
        debts_[first][second] = updated;
    }

    // Records what an expense creates: everyone except the payer owes the payer
    // their share. The payer's own share is not a debt to themselves.
    void record(const Expense &expense) {
        for (const Split &split : expense.splits()) {
            if (split.participant() == expense.paidBy()) {
                continue;
            }
            addDebt(split.participant(), expense.paidBy(), split.amount());
        }
    }

    // Replaces every obligation at once. Only debt simplification uses this: it
    // rewrites the whole book, so applying its result pair by pair would leave
    // whatever the rewrite dropped.
    void replaceWith(const std::vector<Debt> &debts) {
        debts_.clear();
        for (const Debt &debt : debts) {
            addDebt(debt.debtor, debt.creditor, debt.amount);
        }
    }

    // What `debtor` currently owes `creditor`. Zero when they owe nothing, and
    // never negative - a credit in the other direction is not a debt.
    Money::Amount debtOf(const UserId &debtor, const UserId &creditor) const {
        if (debtor == creditor) {
            return 0;
        }

        const bool debtorIsFirst = debtor < creditor;
        const UserId &first = debtorIsFirst ? debtor : creditor;
        const UserId &second = debtorIsFirst ? creditor : debtor;

        const Money::Amount signedAmount = stored(first, second);
        const Money::Amount owed = debtorIsFirst ? signedAmount : -signedAmount;
        return owed > 0 ? owed : 0;
    }

    // Positive: the user is owed this much overall. Negative: the user owes.
    Money::Amount netOf(const UserId &user) const {
        Money::Amount net = 0;

        const auto asFirst = debts_.find(user);
        if (asFirst != debts_.end()) {
            for (const auto &entry : asFirst->second) {
                net -= entry.second;
            }
        }
        for (const auto &entry : debts_) {
            if (entry.first == user) {
                continue;
            }
            const auto asSecond = entry.second.find(user);
            if (asSecond != entry.second.end()) {
                net += asSecond->second;
            }
        }
        return net;
    }

    // Per counterparty, positive meaning that counterparty owes the user.
    std::map<UserId, Money::Amount> obligationsOf(const UserId &user) const {
        std::map<UserId, Money::Amount> obligations;

        const auto asFirst = debts_.find(user);
        if (asFirst != debts_.end()) {
            for (const auto &entry : asFirst->second) {
                obligations[entry.first] -= entry.second;
            }
        }
        for (const auto &entry : debts_) {
            if (entry.first == user) {
                continue;
            }
            const auto asSecond = entry.second.find(user);
            if (asSecond != entry.second.end()) {
                obligations[entry.first] += asSecond->second;
            }
        }
        return obligations;
    }

    bool hasDebtsWith(const UserId &user) const {
        const auto asFirst = debts_.find(user);
        if (asFirst != debts_.end() && !asFirst->second.empty()) {
            return true;
        }
        for (const auto &entry : debts_) {
            if (entry.first == user) {
                continue;
            }
            if (entry.second.find(user) != entry.second.end()) {
                return true;
            }
        }
        return false;
    }

    std::vector<Debt> debts() const {
        std::vector<Debt> result;
        result.reserve(transactionCount());
        for (const auto &outer : debts_) {
            for (const auto &inner : outer.second) {
                if (inner.second > 0) {
                    result.push_back(Debt{outer.first, inner.first, inner.second});
                } else {
                    result.push_back(Debt{inner.first, outer.first, -inner.second});
                }
            }
        }
        return result;
    }

    std::vector<UserId> users() const {
        std::set<UserId> everyone;
        for (const auto &outer : debts_) {
            everyone.insert(outer.first);
            for (const auto &inner : outer.second) {
                everyone.insert(inner.first);
            }
        }
        return std::vector<UserId>(everyone.begin(), everyone.end());
    }

    std::size_t transactionCount() const {
        std::size_t count = 0;
        for (const auto &outer : debts_) {
            count += outer.second.size();
        }
        return count;
    }

    // Total money in flight, counted once per pair.
    Money::Amount totalOutstanding() const {
        Money::Amount total = 0;
        for (const auto &outer : debts_) {
            for (const auto &inner : outer.second) {
                total += inner.second > 0 ? inner.second : -inner.second;
            }
        }
        return total;
    }

    bool empty() const {
        return debts_.empty();
    }

private:
    Money::Amount stored(const UserId &first, const UserId &second) const {
        const auto outer = debts_.find(first);
        if (outer == debts_.end()) {
            return 0;
        }
        const auto inner = outer->second.find(second);
        return inner == outer->second.end() ? 0 : inner->second;
    }

    void forget(const UserId &first, const UserId &second) {
        const auto outer = debts_.find(first);
        if (outer == debts_.end()) {
            return;
        }
        outer->second.erase(second);
        if (outer->second.empty()) {
            debts_.erase(outer);
        }
    }

    // Outer key is the smaller of the two ids; the value is what it owes the
    // inner key.
    std::map<UserId, std::map<UserId, Money::Amount>> debts_;
};

#endif // SPLITWISE_LEDGER_HPP
