#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "app/Splitwise.hpp"
#include "app/UserDirectory.hpp"
#include "errors/AppError.hpp"
#include "models/Expense.hpp"
#include "models/ExpenseDraft.hpp"
#include "models/Group.hpp"
#include "models/Ids.hpp"
#include "models/Ledger.hpp"
#include "models/Split.hpp"
#include "models/User.hpp"
#include "money/Money.hpp"
#include "observers/ConsoleNotifier.hpp"
#include "observers/GroupEvent.hpp"
#include "observers/GroupListener.hpp"
#include "settlement/DebtSimplifier.hpp"
#include "splits/SplitSpec.hpp"
#include "splits/SplitType.hpp"
#include "utils/ConsoleStyle.hpp"

using namespace std;

namespace {

constexpr double LUNCH_RUPEES = 800.0;
constexpr double DINNER_RUPEES = 700.0;
constexpr double COFFEE_RUPEES = 40.0;
constexpr double CYCLE_RUPEES = 3000.0;

struct Checks {
    int passed = 0;
    int failed = 0;

    void expect(bool condition, const string &label) {
        if (condition) {
            Console::ok(label);
            ++passed;
        } else {
            Console::fail(label);
            ++failed;
        }
    }
};

// Runs an action that is expected to refuse, and hands back the reason.
template <typename Action>
bool refuses(Action action, string &reason) {
    try {
        action();
        reason.clear();
        return false;
    } catch (const exception &error) {
        reason = error.what();
        return true;
    }
}

string formatSigned(Money::Amount amount) {
    return (amount > 0 ? "+" : "") + Money::format(amount);
}

string countOf(size_t amount, const string &singular, const string &plural) {
    return to_string(amount) + " " + (amount == 1 ? singular : plural);
}

void printExpense(const Splitwise &app, const Expense &expense) {
    cout << "    " << Console::BOLD << Console::padRight(expense.description(), 8) << Console::RESET
         << Console::padLeft(Money::format(expense.total()), 10) << Console::DIM << "  paid by "
         << Console::padRight(app.user(expense.paidBy()).name(), 8) << Console::RESET << Console::DIM
         << "  [" << expense.id() << "]" << Console::RESET << "\n";

    for (const Split &split : expense.splits()) {
        cout << "        " << Console::padRight(app.user(split.participant()).name(), 9)
             << Console::padLeft(Money::format(split.amount()), 10) << "\n";
    }
}

void printGroupLedger(const Splitwise &app, const Group &group) {
    const Ledger &ledger = group.ledger();

    cout << "    " << Console::BOLD << group.name() << Console::RESET << Console::DIM << "  "
         << countOf(group.members().size(), "member", "members") << " · "
         << countOf(ledger.transactionCount(), "transaction", "transactions") << " · "
         << Money::format(ledger.totalOutstanding()) << " in flight" << Console::RESET << "\n";

    if (ledger.empty()) {
        Console::info("no outstanding debts");
    } else {
        for (const Debt &debt : ledger.debts()) {
            cout << "      " << Console::padRight(app.user(debt.debtor).name(), 9) << "→  "
                 << Console::padRight(app.user(debt.creditor).name(), 9)
                 << Console::padLeft(Money::format(debt.amount), 10) << "\n";
        }
    }

    cout << "      " << Console::DIM << "net ";
    for (const UserId &memberId : group.members()) {
        cout << "· " << app.user(memberId).name() << " "
             << formatSigned(ledger.netOf(memberId)) << " ";
    }
    cout << Console::RESET << "\n";
}

void printMemberBalances(const Splitwise &app, const UserId &userId) {
    const map<UserId, Money::Amount> obligations = app.obligationsOf(userId);

    cout << "    " << Console::BOLD << Console::padRight(app.user(userId).name(), 9)
         << Console::RESET << Console::padLeft(formatSigned(app.netBalanceOf(userId)), 13);

    if (obligations.empty()) {
        cout << Console::DIM << "  settled up" << Console::RESET << "\n";
        return;
    }
    for (const auto &entry : obligations) {
        const string other = app.user(entry.first).name();
        if (entry.second > 0) {
            cout << Console::DIM << "  " << other << " owes "
                 << Money::format(entry.second) << Console::RESET;
        } else {
            cout << Console::DIM << "  owes " << other << " "
                 << Money::format(-entry.second) << Console::RESET;
        }
    }
    cout << "\n";
}

}

int main() {
    Console::banner("SPLITWISE — GROUPS · SPLITS · SETTLEMENT");
    Checks checks;

    // ---------------------------------------------------------------- 1
    Console::section("1. Users");

    Splitwise app;
    const User &aditya = app.createUser("Aditya", "aditya@gmail.com");
    const User &rohit = app.createUser("Rohit", "rohit@gmail.com");
    const User &manish = app.createUser("Manish", "manish@gmail.com");
    const User &saurav = app.createUser("Saurav", "saurav@gmail.com");

    for (const auto &entry : app.directory().users()) {
        cout << "    " << Console::BOLD << Console::padRight(entry.second.name(), 9)
             << Console::RESET << Console::DIM << Console::padRight(entry.first, 8)
             << entry.second.email() << Console::RESET << "\n";
    }

    checks.expect(app.directory().count() == 4, "four users registered");
    checks.expect(aditya.id() == "user1" && saurav.id() == "user4",
                  "ids are generated in order (user1 … user4)");

    // ---------------------------------------------------------------- 2
    Console::section("2. Group");

    const Group &hostel = app.createGroup("Hostel Expenses");
    for (const User *user : {&aditya, &rohit, &manish, &saurav}) {
        app.addMember(hostel.id(), user->id());
    }

    Console::kv("group", hostel.name() + " (" + hostel.id() + ")");
    Console::kv("members", to_string(hostel.members().size()));
    Console::kv("listeners", to_string(hostel.listenerCount()) + " (the facade wired one in)");

    checks.expect(hostel.members().size() == 4, "four members in the group");
    checks.expect(hostel.listenerCount() == 1, "the notifier was subscribed at creation");

    // ---------------------------------------------------------------- 3
    Console::section("3. Two expenses, two split strategies");
    Console::info("the lines below each expense are the observer's output, one per member");

    const Expense &lunch = app.addGroupExpense(
        hostel.id(), ExpenseDraft::equal("Lunch", Money::fromRupees(LUNCH_RUPEES), aditya.id(),
                                        {aditya.id(), rohit.id(), manish.id(), saurav.id()}));

    cout << "\n";
    const Expense &dinner = app.addGroupExpense(
        hostel.id(), ExpenseDraft::exact("Dinner", Money::fromRupees(DINNER_RUPEES), manish.id(),
                                        {aditya.id(), manish.id(), saurav.id()},
                                        {200.0, 300.0, 200.0}));

    cout << "\n";
    printExpense(app, lunch);
    cout << "\n";
    printExpense(app, dinner);

    checks.expect(lunch.shareOf(rohit.id()) == Money::fromRupees(200.0),
                  "equal split of Rs 800 over four is exactly Rs 200 each");
    checks.expect(lunch.shareOf(aditya.id()) + lunch.shareOf(rohit.id()) +
                          lunch.shareOf(manish.id()) + lunch.shareOf(saurav.id()) ==
                      lunch.total(),
                  "the four equal shares add back up to the total");
    checks.expect(dinner.shareOf(manish.id()) == Money::fromRupees(300.0),
                  "exact split keeps the stated Rs 300 for Manish");

    // ---------------------------------------------------------------- 4
    Console::section("4. Group balances");
    printGroupLedger(app, hostel);

    checks.expect(hostel.ledger().debtOf(rohit.id(), aditya.id()) == Money::fromRupees(200.0),
                  "Rohit owes Aditya Rs 200 from lunch");
    checks.expect(hostel.ledger().debtOf(manish.id(), aditya.id()) == 0,
                  "Manish's Rs 200 lunch debt and Rs 200 dinner share cancel exactly");
    checks.expect(hostel.ledger().transactionCount() == 3, "three transactions, not four");
    checks.expect(hostel.ledger().totalOutstanding() == Money::fromRupees(600.0),
                  "Rs 600 is in flight");

    // ---------------------------------------------------------------- 5
    Console::section("5. Simplify the hostel debts");

    const size_t beforeSimplify = hostel.ledger().transactionCount();
    const Money::Amount adityaNet = hostel.ledger().netOf(aditya.id());
    const size_t afterSimplify = app.simplifyDebts(hostel.id());

    cout << "\n";
    printGroupLedger(app, hostel);

    checks.expect(afterSimplify < beforeSimplify,
                  "simplification reduced " + to_string(beforeSimplify) + " transactions to " +
                      to_string(afterSimplify));
    checks.expect(hostel.ledger().netOf(aditya.id()) == adityaNet,
                  "nobody's net changed, only who pays whom");
    checks.expect(hostel.ledger().obligationsOf(saurav.id()).size() == 1,
                  "Saurav's two Rs 200 payments to two people became one payment of Rs 400");

    // ---------------------------------------------------------------- 6
    Console::section("6. A group whose debts form a cycle");
    Console::info("three people each paid Rs 3000 for the next one: everyone is square");

    const Group &trip = app.createGroup("Goa Trip");
    for (const User *user : {&aditya, &manish, &saurav}) {
        app.addMember(trip.id(), user->id());
    }

    const Money::Amount cycleAmount = Money::fromRupees(CYCLE_RUPEES);
    app.addGroupExpense(trip.id(), ExpenseDraft::exact("Flight", cycleAmount, aditya.id(),
                                                      {manish.id()}, {CYCLE_RUPEES}));
    app.addGroupExpense(trip.id(), ExpenseDraft::exact("Hotel", cycleAmount, manish.id(),
                                                      {saurav.id()}, {CYCLE_RUPEES}));
    app.addGroupExpense(trip.id(), ExpenseDraft::exact("Food", cycleAmount, saurav.id(),
                                                      {aditya.id()}, {CYCLE_RUPEES}));

    cout << "\n";
    printGroupLedger(app, trip);
    const size_t cycleBefore = trip.ledger().transactionCount();

    cout << "\n";
    const size_t cycleAfter = app.simplifyDebts(trip.id());
    cout << "\n";
    printGroupLedger(app, trip);

    checks.expect(cycleBefore == 3, "the cycle starts as three transactions");
    checks.expect(cycleAfter == 0, "simplification removes all three: every net was zero");
    checks.expect(trip.ledger().empty(), "the trip ledger is empty");

    // ---------------------------------------------------------------- 7
    Console::section("7. A personal expense — the bug this project fixes");
    Console::info("original: split said Rs 20 each, but the balance recorded the full Rs 40");

    const Expense &coffee = app.addPersonalExpense(ExpenseDraft::equal(
        "Coffee", Money::fromRupees(COFFEE_RUPEES), rohit.id(), {rohit.id(), saurav.id()}));

    cout << "\n";
    printExpense(app, coffee);

    checks.expect(coffee.isPersonal(), "the expense carries no group id");
    checks.expect(coffee.shareOf(saurav.id()) == Money::fromRupees(20.0),
                  "Saurav owes his half, Rs 20");
    checks.expect(app.personalLedger().debtOf(saurav.id(), rohit.id()) == Money::fromRupees(20.0),
                  "the personal ledger recorded Rs 20, not Rs 40");

    // ---------------------------------------------------------------- 8
    Console::section("8. Every user, across every book");

    Money::Amount netTotal = 0;
    for (const User *user : {&aditya, &rohit, &manish, &saurav}) {
        printMemberBalances(app, user->id());
        netTotal += app.netBalanceOf(user->id());
    }
    Console::divider(70);
    Console::kv("sum of nets", formatSigned(netTotal));

    checks.expect(app.netBalanceOf(aditya.id()) == Money::fromRupees(400.0),
                  "Aditya is owed Rs 400 overall");
    checks.expect(app.netBalanceOf(rohit.id()) == -Money::fromRupees(180.0),
                  "Rohit owes Rs 200 in the group and is owed Rs 20 for coffee");
    checks.expect(app.netBalanceOf(manish.id()) == Money::fromRupees(200.0),
                  "Manish paid for dinner, so he ends up a creditor");
    checks.expect(app.netBalanceOf(saurav.id()) == -Money::fromRupees(420.0),
                  "Saurav owes Rs 400 in the group and Rs 20 for coffee");
    checks.expect(netTotal == 0, "every rupee owed is a rupee someone is owed");
    checks.expect(app.obligationsOf(aditya.id()).size() == 1,
                  "after simplification Aditya deals with one person, not three");

    // ---------------------------------------------------------------- 9
    Console::section("9. Leaving a group with an open debt");

    string reason;
    if (refuses([&] { app.removeMember(hostel.id(), rohit.id()); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("still owe") != string::npos,
                      "removal refused because Rohit still owes");
        checks.expect(reason.find("Rs 200.00") != string::npos,
                      "the refusal says how much is outstanding");
    }
    checks.expect(hostel.isMember(rohit.id()), "Rohit is still a member after the refusal");

    // The counterparty moved during simplification: Rohit owed Aditya before,
    // and owes Manish now. The original demo settled Rohit with Manish while he
    // still owed Aditya, so the debt survived and the removal failed twice.
    checks.expect(hostel.ledger().debtOf(rohit.id(), aditya.id()) == 0 &&
                      hostel.ledger().debtOf(rohit.id(), manish.id()) == Money::fromRupees(200.0),
                  "simplification moved Rohit's Rs 200 from Aditya to Manish");

    app.settleInGroup(hostel.id(), rohit.id(), manish.id(), Money::fromRupees(200.0));
    cout << "\n";
    app.removeMember(hostel.id(), rohit.id());

    checks.expect(!hostel.isMember(rohit.id()), "with the debt settled, Rohit can leave");
    checks.expect(hostel.members().size() == 3, "three members remain");
    checks.expect(!hostel.ledger().hasDebtsWith(rohit.id()), "no obligation mentions him");
    checks.expect(hostel.expenseCount() == 2, "the expense history keeps both expenses");
    checks.expect(app.group(hostel.id()).expense(lunch.id()).paidBy() == aditya.id(),
                  "a removed member is still named on the expenses they paid for");

    // ---------------------------------------------------------------- 10
    Console::section("10. Settling the coffee");

    app.settlePersonally(saurav.id(), rohit.id(), Money::fromRupees(20.0));
    cout << "\n";
    printMemberBalances(app, saurav.id());
    printMemberBalances(app, rohit.id());

    checks.expect(app.personalLedger().empty(), "the personal book is empty");
    checks.expect(app.netBalanceOf(rohit.id()) == 0,
                  "Rohit is square everywhere: he paid his half, and left with nothing owed");

    // ---------------------------------------------------------------- 11
    Console::section("11. Refusals");

    const Group &scratch = app.createGroup("Refusal Tests");
    for (const User *user : {&aditya, &rohit, &manish}) {
        app.addMember(scratch.id(), user->id());
    }

    if (refuses([&] { app.createUser("Impostor", "ROHIT@gmail.com"); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("already registered") != string::npos,
                      "a duplicate email is refused, case-insensitively");
    }

    if (refuses([&] { app.user("user99"); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("unknown user") != string::npos, "an unknown user is refused");
    }

    if (refuses([&] { app.addMember(scratch.id(), aditya.id()); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("already a member") != string::npos,
                      "adding an existing member is refused");
    }

    if (refuses([&] { app.removeMember(scratch.id(), saurav.id()); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("not a member") != string::npos,
                      "removing someone who never joined is refused");
    }

    if (refuses(
            [&] {
                app.addGroupExpense(scratch.id(),
                                    ExpenseDraft::equal("Ghost", Money::fromRupees(100.0),
                                                        saurav.id(), {aditya.id()}));
            },
            reason)) {
        Console::warn(reason);
        checks.expect(reason.find("payer") != string::npos,
                      "a payer outside the group is refused");
    }

    if (refuses(
            [&] {
                app.addGroupExpense(scratch.id(),
                                    ExpenseDraft::equal("Ghost", Money::fromRupees(100.0),
                                                        aditya.id(), {saurav.id()}));
            },
            reason)) {
        Console::warn(reason);
        checks.expect(reason.find("cannot share") != string::npos,
                      "a participant outside the group is refused");
    }

    if (refuses(
            [&] {
                app.addGroupExpense(scratch.id(),
                                    ExpenseDraft::exact("Short", Money::fromRupees(700.0),
                                                        aditya.id(),
                                                        {aditya.id(), rohit.id(), manish.id()},
                                                        {200.0, 300.0, 100.0}));
            },
            reason)) {
        Console::warn(reason);
        checks.expect(reason.find("but the expense is") != string::npos,
                      "an exact split that does not add up is refused");
    }

    if (refuses(
            [&] {
                app.addGroupExpense(scratch.id(),
                                    ExpenseDraft::exact("Mismatch", Money::fromRupees(700.0),
                                                        aditya.id(),
                                                        {aditya.id(), rohit.id(), manish.id()},
                                                        {200.0, 500.0}));
            },
            reason)) {
        Console::warn(reason);
        checks.expect(reason.find("amount(s) for") != string::npos,
                      "an exact split with the wrong number of amounts is refused");
    }

    if (refuses(
            [&] {
                app.addGroupExpense(scratch.id(),
                                    ExpenseDraft::percentage("Ninety", Money::fromRupees(700.0),
                                                             aditya.id(),
                                                             {aditya.id(), rohit.id(), manish.id()},
                                                             {30.0, 30.0, 30.0}));
            },
            reason)) {
        Console::warn(reason);
        checks.expect(reason.find("expected 100%") != string::npos,
                      "percentages that do not total 100 are refused");
    }

    if (refuses(
            [&] {
                app.addGroupExpense(scratch.id(),
                                    ExpenseDraft::percentage("Tiny", Money::fromRupees(0.01),
                                                             aditya.id(),
                                                             {aditya.id(), rohit.id(), manish.id()},
                                                             {50.0, 50.0, 0.0}));
            },
            reason)) {
        Console::warn(reason);
        checks.expect(reason.find("too small") != string::npos,
                      "a total too small for the percentages is refused");
    }

    ExpenseDraft equalWithValues = ExpenseDraft::equal("Wrong", Money::fromRupees(100.0),
                                                       aditya.id(), {aditya.id(), rohit.id()});
    equalWithValues.split = SplitSpec{SplitType::Equal, {1.0, 2.0}};
    if (refuses([&] { app.addGroupExpense(scratch.id(), equalWithValues); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("takes no values") != string::npos,
                      "an equal split carrying values is refused, not silently ignored");
    }

    ExpenseDraft unknownType = ExpenseDraft::equal("Unknown", Money::fromRupees(100.0),
                                                   aditya.id(), {aditya.id(), rohit.id()});
    unknownType.split.type = static_cast<SplitType>(99);
    if (refuses([&] { app.addGroupExpense(scratch.id(), unknownType); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("unknown split type") != string::npos,
                      "an unknown split type is refused, not downgraded to equal");
    }

    if (refuses(
            [&] {
                app.addGroupExpense(scratch.id(),
                                    ExpenseDraft::equal("Twice", Money::fromRupees(100.0),
                                                        aditya.id(),
                                                        {aditya.id(), rohit.id(), rohit.id()}));
            },
            reason)) {
        Console::warn(reason);
        checks.expect(reason.find("appears twice") != string::npos,
                      "the same participant twice in one expense is refused");
    }

    if (refuses(
            [&] {
                app.addGroupExpense(scratch.id(),
                                    ExpenseDraft::equal("Zero", 0, aditya.id(),
                                                        {aditya.id(), rohit.id()}));
            },
            reason)) {
        Console::warn(reason);
        checks.expect(reason.find("must be positive") != string::npos,
                      "a zero expense is refused");
    }

    if (refuses([&] { app.settleInGroup(scratch.id(), aditya.id(), aditya.id(),
                                        Money::fromRupees(10.0)); },
                reason)) {
        Console::warn(reason);
        checks.expect(reason.find("themselves") != string::npos,
                      "settling with yourself is refused");
    }

    if (refuses([&] { app.settleInGroup(scratch.id(), aditya.id(), rohit.id(), 0); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("must be positive") != string::npos,
                      "a zero settlement is refused");
    }

    if (refuses([&] { app.settleInGroup(scratch.id(), aditya.id(), saurav.id(),
                                        Money::fromRupees(10.0)); },
                reason)) {
        Console::warn(reason);
        checks.expect(reason.find("not a member") != string::npos,
                      "settling with a non-member is refused");
    }

    if (refuses([&] { app.simplifyDebts("group99"); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("unknown group") != string::npos,
                      "simplifying an unknown group is refused");
    }

    checks.expect(scratch.expenseCount() == 0,
                  "none of the refused drafts reached the expense book");

    // ---------------------------------------------------------------- 12
    Console::section("12. Equal split of Rs 800 among three");

    const Expense &uneven = app.addGroupExpense(
        scratch.id(),
        ExpenseDraft::equal("Uneven", Money::fromRupees(LUNCH_RUPEES), aditya.id(),
                            {aditya.id(), rohit.id(), manish.id()}));

    cout << "\n";
    printExpense(app, uneven);

    checks.expect(Money::format(uneven.shareOf(aditya.id())) == "Rs 266.67",
                  "the leftover paise land on the first participant: Rs 266.67");
    checks.expect(uneven.shareOf(aditya.id()) + uneven.shareOf(rohit.id()) +
                          uneven.shareOf(manish.id()) == uneven.total(),
                  "and the shares still add up to exactly Rs 800.00");
    checks.expect(Money::format(-Money::fromRupees(COFFEE_RUPEES)) == "-Rs 40.00",
                  "negative money formats with the sign outside the symbol");

    // ---------------------------------------------------------------- 13
    Console::section("13. Final state");

    for (const auto &entry : app.groups()) {
        cout << "    " << Console::padRight(entry.second.name(), 16) << Console::DIM
             << Console::padRight(entry.first, 9) << entry.second.members().size()
             << " members · " << countOf(entry.second.expenseCount(), "expense", "expenses")
             << " · "
             << countOf(entry.second.ledger().transactionCount(), "transaction", "transactions")
             << Console::RESET << "\n";
    }
    Console::divider(70);
    Console::kv("users", to_string(app.directory().count()));
    Console::kv("groups", to_string(app.groupCount()));
    Console::kv("personal expenses", to_string(app.personalExpenses().size()));
    Console::kv("personal ledger", app.personalLedger().empty() ? "settled" : "open");

    Console::divider(70);
    if (checks.failed == 0) {
        Console::ok(to_string(checks.passed) + " checks passed");
        return 0;
    }
    Console::fail(to_string(checks.failed) + " of " + to_string(checks.passed + checks.failed) +
                  " checks failed");
    return 1;
}
