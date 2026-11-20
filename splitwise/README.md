# Splitwise — Groups, Splits, Settlement

LLD exercise: shared expenses in groups, three ways to split a bill, personal expenses, and a debt simplifier — behind one facade.

Starting point was a 739-line single-file snippet (`temp.tmp`). This version keeps the same domain and the same demo story, fixes the defects listed below, and uses only patterns already practiced elsewhere in this repo.

## Patterns Used

| Pattern | Where | What it does here |
|---|---|---|
| **Facade** | `app/Splitwise.hpp` | One object hides the directory, the groups, the personal book and the notifier. `addGroupExpense` alone decides: validate, split, record in the book, update the ledger, tell every member |
| **Observer** | `observers/` | `Group` is the subject; `GroupEvent` is what happened; `ConsoleNotifier` is the only listener |
| **Strategy** | `splits/` | `EqualSplit`, `ExactSplit`, `PercentageSplit` — each owns its own configuration and answers one question: what does each person owe? |
| **Factory** | `splits/SplitFactory.hpp` | `SplitSpec` → strategy, returned as `unique_ptr`. An unknown type throws instead of quietly becoming an equal split |

That is the whole list.

### What was deliberately left out

| Removed | Why |
|---|---|
| **Singleton** for `Splitwise` | `getInstance()` returned a leaked `new Splitwise`, was not thread-safe, and could not be reset — one run's state became the next run's starting point. The caller owns the facade now. |
| `vector<User*> members` in `Group` | A group held borrowings from the directory, so the two could disagree, and every name lookup dereferenced a pointer that `getUserByuserId` might have returned as `nullptr`. A member is a `UserId`; names come from the directory at the edge. |
| `const vector<double>& values = {}` on the split strategy | One parameter meant rupees for `ExactSplit` and percent for `PercentageSplit`, with a *default argument on a virtual function* resolved from the static type. Any value could reach any strategy. Each strategy takes its own configuration in its constructor now. |
| The symmetric balance matrix | `balances[A][B] = +200` and `balances[B][A] = -200` described one debt twice, so every write had to update both, every read had to pick a sign, and the simplifier had to remember to "only process positive amounts to avoid double counting". See [the ledger](#the-ledger-one-number-per-pair). |
| `User::balances` / `updateBalance` | A person's money lived on the person, so a `User` could not be printed, copied or moved without carrying someone else's debts. `User` is now just identity. |

## The money bug

```cpp
// temp.tmp:24
double amount;
// temp.tmp:45 split
double share = totalAmount / userIds.size();
```

Splitting Rs 800 four ways gives `199.99999999999997` per head, so the four shares do not add back up to the 800 that was paid. Every comparison then needs a fudge (`abs(x) < 0.01`), and the fudge is the only thing keeping the books looking balanced.

```cpp
// money/Money.hpp
using Amount = long long;              // paise
constexpr Amount PAISE_PER_RUPEE = 100;
```

- A share is a whole number of paise, so `sum(shares) == total` is checked by construction, not by tolerance.
- Rupees exist only at two edges: input (`fromRupees`) and output (`format`).
- `format` builds `"Rs 266.67"` from integers, so no `double` ever formats money either.
- `PercentageSplit` stores basis points, so "the shares add up to 100%" is an integer comparison.

## The ledger: one number per pair

```cpp
// temp.tmp
groupBalances[A][B] += amount;
groupBalances[B][A] -= amount;
```

One debt, two cells. `Ledger` normalises the pair instead: the ids are ordered, the smaller first, and the stored value is what the **first owes the second**. A negative value means the second owes the first, and a pair that cancels to zero is erased.

| Operation | `temp.tmp` | Here |
|---|---|---|
| B owes A 200 | two cells to write | `addDebt(B, A, 200)` → one cell |
| A owes B 200 too | two cells to re-check for zero | `addDebt(A, B, 200)` → the same cell, which becomes 0 and is erased |
| Who owes what | pick a sign convention, per cell | `debtOf(debtor, creditor)`, always non-negative |
| Net position | scan the row | `netOf(user)` |
| Simplify | iterate positive cells only, or double count | one entry per pair |

The demo's `Dinner` expense is exactly this case: Manish's Rs 200 lunch debt and Aditya's Rs 200 dinner share are the same pair, so the second write erases the first. Three obligations remain, not four.

## Structure

```
splitwise/
├── main.cpp                          # Composition root + deterministic demo (59 self-checks)
│
├── models/
│   ├── Ids.hpp                       # UserId / GroupId / ExpenseId aliases
│   ├── User.hpp                      # Identity only: id, name, validated email
│   ├── Split.hpp                     # One participant's share, in paise
│   ├── Expense.hpp                   # Immutable; refuses shares that do not sum to the total
│   ├── ExpenseDraft.hpp              # What a caller supplies; the one place a draft becomes an Expense
│   ├── Ledger.hpp                    # Debts for one group or the personal book, one entry per pair
│   └── Group.hpp                     # Members, expense book, ledger, subscribers
│
├── splits/                           # STRATEGY
│   ├── SplitType.hpp
│   ├── SplitStrategy.hpp             # Interface + the checks all strategies share
│   ├── EqualSplit.hpp                # Leftover paise to the first participants, in order
│   ├── ExactSplit.hpp                # Stated rupees, checked against the total
│   ├── PercentageSplit.hpp           # Basis points, rounded, last share absorbs the remainder
│   ├── SplitSpec.hpp                 # "which rule, with which numbers", built by named ctors
│   └── SplitFactory.hpp              # FACTORY: SplitSpec -> unique_ptr<SplitStrategy>
│
├── settlement/
│   └── DebtSimplifier.hpp            # Greedy max-creditor / max-debtor netting
│
├── observers/                        # OBSERVER
│   ├── GroupEvent.hpp                # ExpenseAdded / Settlement / DebtsSimplified, ids only
│   ├── GroupListener.hpp             # Interface: onGroupEvent
│   └── ConsoleNotifier.hpp           # One line per recipient; borrows the directory for names
│
├── app/
│   ├── UserDirectory.hpp             # Owns users; generated ids; unique email
│   └── Splitwise.hpp                 # FACADE: users, groups, personal book, balances
│
├── errors/
│   └── AppError.hpp                  # One refusal type
│
├── money/
│   └── Money.hpp                     # Integer paise, conversion at the edges
│
└── utils/
    └── ConsoleStyle.hpp              # Colored, UTF-8-width-aware output
```

## Build & Run

```bash
g++ -std=c++26 -Wall -Wextra -o main main.cpp && ./main
```

No dependencies, no build system, single translation unit. The demo is deterministic — no clocks, no randomness.

## Flow

```
Splitwise::addGroupExpense(groupId, draft)
    │
    ├─ 1. directory.get(payer / participants)   # unknown user? refuse, with the id
    ├─ 2. group.addExpense(draft)
    │       ├─ payer and participants are members?      # refuse naming the id
    │       ├─ id = groupId + "-e" + sequence           # sequence advances only on success
    │       ├─ draft.build(id, groupId)
    │       │     ├─ SplitFactory::create(spec)          # unknown type? refuse
    │       │     ├─ strategy.split(total, participants) # shares always sum to total
    │       │     └─ Expense(...)                       # re-checks the sum, no duplicates
    │       ├─ ledger.record(expense)                   # everyone but the payer owes the payer
    │       ├─ expenses.emplace(...)
    │       └─ publish(ExpenseAdded)                    # every member, via every listener
    └─ 3. return the stored expense
```

Three refusals happen before anything is written, so a rejected expense leaves no id, no shares, and no debt behind — the harness asserts that.

## Splits

| Strategy | Input | Rounding rule |
|---|---|---|
| `EqualSplit` | nothing | `total / heads` each; the leftover paise go one each to the first participants in the order given |
| `ExactSplit` | rupees, one per participant | none — the stated amounts must sum to the total exactly, or the expense is refused |
| `PercentageSplit` | percent, one per participant | each share rounded to the nearest paisa, the last participant absorbs the remainder |

```
Rs 800, 4 people, equal      → 200.00 · 200.00 · 200.00 · 200.00
Rs 800, 3 people, equal      → 266.67 · 266.67 · 266.66        (2 leftover paise)
Rs 700, 3 people, exact      → 200.00 · 300.00 · 200.00
Rs 700, 3 people, 40/40/20   → 280.00 · 280.00 · 140.00
```

The old version had a `//validations` comment in `ExactSplit` and `PercentageSplit`, and no validations. `values[i]` ran off the end of the vector whenever the caller supplied fewer amounts than participants.

## Simplification

Only the net position matters, not which expense created it.

```
Hostel Expenses after Lunch + Dinner

    Rohit    →  Aditya    Rs 200.00        nets: Aditya +400
    Saurav   →  Aditya    Rs 200.00              Rohit  -200
    Saurav   →  Manish    Rs 200.00              Manish +200
                                                 Saurav -400
    3 transactions, Rs 600 in flight

after simplifyDebts()

    Saurav   →  Aditya    Rs 400.00        same nets
    Rohit    →  Manish    Rs 200.00
    2 transactions
```

Saurav's two payments became one. Rohit's Rs 200 did not disappear — it moved from Aditya to Manish, which is what netting does. That matters for the next section.

Two honest limits:

- **Greedy, not optimal.** The minimum-transaction problem is NP-hard. This pass produces at most `n - 1` payments for `n` non-zero nets, which is fine for a group of friends and is not provably the minimum.
- **Nets must balance.** The simplifier refuses a ledger whose nets do not sum to zero rather than producing a plausible-looking wrong answer.

## Schema and logic fixes

| Original | Problem | Now |
|---|---|---|
| `double amount` | 800/4 ≠ 800; every comparison needed an epsilon | integer paise; `Expense` refuses shares that do not sum to the total |
| `ExactSplit` / `PercentageSplit` `//validations` | comment only; `values[i]` out of bounds, wrong-count input accepted | both validate count, sign and sum; percentages stored as basis points |
| `EqualSplit` | divided by `userIds.size()` with no check | refuses zero participants; states where the leftover paise go |
| `SplitStrategy::calculateSplit(..., const vector<double>& values = {})` | default argument on a virtual function, one parameter with two meanings | each strategy takes its own configuration in its constructor |
| `SplitFactory::getSplitStrategy` returned `new` and had `default: return new EqualSplit()` | leaked, and a typo quietly became an equal split | returns `unique_ptr`; unknown type throws |
| `groupBalances[A][B]` / `[B][A]` | one debt stored twice; both cells had to be written and re-checked | `Ledger` stores one signed number per ordered pair |
| `Group::removeMember` | erased from `members` **while iterating it**, then erased the user from `groupBalances` but not from the expenses | checks `ledger.hasDebtsWith`, then erases the id; history keeps the removed member |
| `getUserByuserId` returned `nullptr`; callers dereferenced it | null dereference on a typo | `directory.get()` throws, `find()` is the only nullable form |
| `Splitwise::getInstance()` | leaked `new`, unsynchronised, no way to reset | caller-owned facade, copy deleted |
| `Splitwise::~Splitwise` never existed | users, groups and individual expenses leaked | values in maps; no manual `delete` anywhere |
| Inconsistent failure | `addExpense` threw, `settlePayment` printed and returned false, `addUserToGroup` silently did nothing | one `AppError`; every refusal has a reason |
| `addIndividualExpense` credited the payer the full amount | the split said Rs 20 each, the balance recorded Rs 40 | the personal book applies the same split as a group expense |
| `addUserToGroup(userId, groupId)` with unknown ids | silent no-op that looked like success | unknown user / group is refused, and duplicate membership too |
| Expense ids from a static counter | ids collided across runs and said nothing | `<groupId>-e<n>`, per group, generated only on success |
| `User` email never checked for duplicates | two users could be the same person | directory refuses a duplicate email, case-insensitively |
| `showUserBalance` returned silently for an unknown user | a typo looked like a zero balance | `get()` throws |

## Edge cases handled

| Case | Behaviour |
|---|---|
| Split total not divisible by head count | leftover paise distributed one each; sum still exact |
| Exact amounts that do not sum to the total | refused, naming both figures |
| Exact split with the wrong number of amounts | refused |
| Percentages that do not total 100 | refused, naming the total |
| Percentage share too large to multiply safely | refused instead of overflowing |
| Total too small for the percentages | refused, not a negative share |
| Zero or negative expense | refused |
| Duplicate participant in one expense | refused |
| Person listed outside the group | refused for payer and for participants |
| Payer not a participant | allowed — paying for other people is the normal case |
| Payer is a participant | their own share creates no debt to themselves |
| Settlement larger than the debt | allowed; the excess becomes a credit in the other direction |
| Settlement to yourself, or non-positive | refused |
| Removing a member who owes or is owed | refused, with the amount and direction |
| Removing a member whose pairs net to zero but are still open | refused — open obligations still name them |
| Removing a member with no obligations | allowed; the expense history keeps their name |
| Duplicate member | refused |
| Two groups with the same member pair | separate ledgers, no cross-talk |
| Unknown group / user / expense id | refused, naming the id |
| Equal split carrying values | refused, not silently ignored |
| Unknown split type | refused, not downgraded |
| Duplicate email, different case | refused |
| A cycle of equal debts | simplifies to zero transactions |
| Empty ledger | simplifies to nothing, not an error |

## Limitations

Deliberate scope cuts, not oversights:

- **No persistence.** Users, groups, expenses and ledgers live in memory.
- **No editing or deleting an expense.** Immutability is the point; a correction is a new expense or a settlement.
- **Currency is one.** Rupees with two decimals; no currency conversion.
- **Simplification is greedy**, not provably minimal.
- **Membership is flat.** No admins, invitations, or "who may add whom".
- **The demo's group balances assume trusted input.** Multi-tenant access control is out of scope; there is no session or owner check.
- **`notify()` prints.** The listener is a real observer with real events, but delivery is `std::cout`.
- **The facade returns references** into its own storage. They stay valid while the facade is alive, and no longer.

## Verification

```bash
g++ -std=c++26 -Wall -Wextra -o main main.cpp && ./main
```

| Check | Result |
|---|---|
| Compiler warnings | none (`-Wall -Wextra`) |
| Demo self-checks | 59/59 passed, exit 0 |
| Adversarial harness (13,780 assertions: ledger normalisation, random split arithmetic over 8 head counts, 300 random simplifications, refusal atomicity, id sequencing, personal book isolation) | 13,780/13,780 passed |
| Leak audit (`operator new`/`delete` counter), demo | `new=399 delete=399 leaked=0` |
| Leak audit, harness | `new=65265 delete=65265 leaked=0` |

The harness is a throwaway file, not part of the repo. It found two real things while being written: the percentage split's "too small" guard was placed *after* the share was constructed, so the confusing `Split` message won the race; and the demo's own removal story settled Rohit with Aditya after simplification had moved the debt to Manish — the same mistake as the original snippet, reproduced from memory.

## Demo walkthrough

1. **Four users.** Aditya, Rohit, Manish, Saurav with generated ids `user1`…`user4`.
2. **Group.** `Hostel Expenses`, four members, one listener wired in by the facade.
3. **Two expenses.** Lunch Rs 800 equal (four shares of exactly 200) and Dinner Rs 700 exact (200/300/200, paid by Manish). Every member is notified, once per event.
4. **Group balances.** Three transactions, not four: Manish's lunch debt and dinner share are the same pair and cancel. Nets: Aditya +400, Rohit −200, Manish +200, Saurav −400.
5. **Simplification.** 3 → 2. Saurav's two payments become one; Rohit's debt moves to Manish.
6. **A cycle.** `Goa Trip`: three Rs 3000 expenses, each paid for the next person. Three transactions, every net zero, so simplification removes all three — the clearest case for why netting works.
7. **Personal expense.** Coffee Rs 40, split 50/50, and the book records Rs 20 — the original recorded the full 40.
8. **Every user, every book.** Per-user nets and counterparties, merged across groups and the personal book; the nets sum to zero.
9. **Leaving a group.** Rohit's removal is refused with "they still owe Rs 200.00"; the ledger shows he owes Manish now, not Aditya; after settling he leaves, and the expense history still names him.
10. **Settling the coffee.** The personal book empties.
11. **Refusals.** Duplicate email, unknown user, duplicate member, unknown member, non-member payer, non-member participant, exact split that does not add up, wrong count, 90%, too-small total, equal split with values, unknown split type, duplicate participant, zero expense, self-settlement, zero settlement, settlement with a non-member, unknown group — each prints the actual reason, and the refusal group stays empty.
12. **Rs 800 among three.** Rs 266.67 / 266.67 / 266.66, summing to exactly Rs 800.00.
13. **Final state.** Per-group members, expenses and transactions.

## Extending

- **New split rule** — a `SplitType` enumerator, a `SplitSpec` named constructor, one class implementing `split`, one `case` in the factory.
- **New listener** — implement `GroupListener::onGroupEvent` and subscribe in `Splitwise::createGroup`. A `LedgerWriter`, an audit log, or a push notifier costs one class and one line.
- **New event kind** — an enumerator in `GroupEvent::Kind`, a publish call, and a `case` in the notifier.
- **Persistence** — the models are values with validating constructors, so they map to rows without pointer fix-ups.
- **A better simplifier** — `DebtSimplifier::simplify` is a pure `Ledger → vector<Debt>` function, so an optimal or per-group-partitioned version replaces it without touching `Group`.
