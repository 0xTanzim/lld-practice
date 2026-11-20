#ifndef SPLITWISE_DEBT_SIMPLIFIER_HPP
#define SPLITWISE_DEBT_SIMPLIFIER_HPP

#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../models/Ledger.hpp"
#include "../money/Money.hpp"

// Collapses a ledger into the fewest payments a greedy pass can find.
//
// The rule is netting: only what each person is up or down overall matters, not
// which expense created it. The largest creditor and the largest debtor settle
// against each other, and both totals shrink.
//
// Two honest limits, stated because the original claimed more than it did:
//
//   - this is greedy, not optimal. The minimum-transaction problem is NP-hard,
//     and greedy produces at most (participants - 1) payments, which is enough
//     for a group of friends and is not provably the minimum.
//   - the original worked on a symmetric matrix and had to remember to "only
//     process positive amounts to avoid double counting". A normalized Ledger
//     has one entry per pair, so netting is a single pass.
class DebtSimplifier {
public:
    static std::vector<Debt> simplify(const Ledger &ledger) {
        std::map<UserId, Money::Amount> net;
        Money::Amount total = 0;
        for (const UserId &user : ledger.users()) {
            net[user] = ledger.netOf(user);
            total += net[user];
        }

        if (total != 0) {
            throw AppError("debt simplifier: ledger does not balance (" + Money::format(total) +
                           " left over)");
        }

        std::vector<std::pair<UserId, Money::Amount>> creditors;
        std::vector<std::pair<UserId, Money::Amount>> debtors;
        for (const auto &entry : net) {
            if (entry.second > 0) {
                creditors.emplace_back(entry.first, entry.second);
            } else if (entry.second < 0) {
                debtors.emplace_back(entry.first, -entry.second);
            }
        }

        // Largest first, and by id when two ties: the order has to be stable or
        // the same ledger would simplify differently on every run.
        const auto largestFirst = [](const std::pair<UserId, Money::Amount> &left,
                                     const std::pair<UserId, Money::Amount> &right) {
            if (left.second != right.second) {
                return left.second > right.second;
            }
            return left.first < right.first;
        };
        std::sort(creditors.begin(), creditors.end(), largestFirst);
        std::sort(debtors.begin(), debtors.end(), largestFirst);

        std::vector<Debt> simplified;
        simplified.reserve(creditors.size() + debtors.size());

        std::size_t creditorIndex = 0;
        std::size_t debtorIndex = 0;
        while (creditorIndex < creditors.size() && debtorIndex < debtors.size()) {
            const Money::Amount amount = std::min(creditors[creditorIndex].second,
                                                  debtors[debtorIndex].second);
            simplified.push_back(
                Debt{debtors[debtorIndex].first, creditors[creditorIndex].first, amount});

            creditors[creditorIndex].second -= amount;
            debtors[debtorIndex].second -= amount;

            if (creditors[creditorIndex].second == 0) {
                ++creditorIndex;
            }
            if (debtors[debtorIndex].second == 0) {
                ++debtorIndex;
            }
        }

        return simplified;
    }
};

#endif // SPLITWISE_DEBT_SIMPLIFIER_HPP
