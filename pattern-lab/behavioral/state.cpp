#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

// STATE: behaviour lives in one object per state, and every event is dispatched
// to whichever state is current.
//
// The alternative is a `switch (state_)` inside every event method: the same
// table copied five times, and a sixth state means editing all five. Here the
// table is still a table - each row is a class, and each class answers with the
// row that comes next.
//
// What a state may do is deliberately narrow. It cannot assign the balance, the
// stock or the price; it asks the machine, and the machine refuses anything that
// would break an invariant. That is what keeps a five-state, five-event table
// honest: no state can put the machine into a shape the others do not expect.

class VendingMachine;

enum class StateKind { NoCoin, HasCoin, Dispensing, SoldOut };

const char *name(StateKind kind) {
    switch (kind) {
        case StateKind::NoCoin:
            return "NO_COIN";
        case StateKind::HasCoin:
            return "HAS_COIN";
        case StateKind::Dispensing:
            return "DISPENSING";
        case StateKind::SoldOut:
            return "SOLD_OUT";
    }
    return "UNKNOWN";
}

string countOf(int count, const char *singular, const char *plural) {
    return to_string(count) + " " + (count == 1 ? singular : plural);
}

// A state's whole answer: where to go next, and what happened. No printing and
// no formatting inside a state - it returns data and the caller decides.
struct Transition {
    StateKind next;
    string message;
};

class VendingState {
public:
    // States are deleted through this pointer, so the destructor has to be
    // virtual. The original left it out and leaked all four of them.
    virtual ~VendingState() = default;

    virtual Transition insertCoin(VendingMachine &machine, int rupees) = 0;
    virtual Transition selectItem(VendingMachine &machine) = 0;
    virtual Transition dispense(VendingMachine &machine) = 0;
    virtual Transition refund(VendingMachine &machine) = 0;
    virtual Transition refill(VendingMachine &machine, int items) = 0;
};

class NoCoinState;
class HasCoinState;
class DispensingState;
class SoldOutState;

// What one event did: where it started, where it left, and why.
struct Step {
    StateKind from;
    StateKind to;
    string message;
};

// The context. It owns the state objects, the stock and the money, and it is the
// only thing that can change any of them. The original exposed setInsertedCoin,
// decrementItemCount and setPrice to every state, so each state could undo
// another state's assumptions - including repricing the machine mid-purchase.
class VendingMachine {
public:
    VendingMachine(int items, int priceEach);
    ~VendingMachine();

    VendingMachine(const VendingMachine &) = delete;
    VendingMachine &operator=(const VendingMachine &) = delete;

    Step insertCoin(int rupees);
    Step selectItem();
    Step dispense();
    Step refund();
    Step refill(int items);

    StateKind state() const { return current_; }
    int items() const { return items_; }
    int balance() const { return balance_; }
    int price() const { return price_; }

    // What a state is allowed to ask for. Each one refuses an operation that
    // would break an invariant, and none of them is a setter: the machine, not
    // the state, decides what a legal balance or item count looks like.
    void credit(int rupees);
    int settle();
    void releaseItem();
    int withdrawBalance();
    void restock(int items);

private:
    VendingState *handler(StateKind kind) const;
    Step apply(Transition transition);

    int items_;
    const int price_;
    int balance_ = 0;
    StateKind current_ = StateKind::NoCoin;

    unique_ptr<NoCoinState> noCoin_;
    unique_ptr<HasCoinState> hasCoin_;
    unique_ptr<DispensingState> dispensing_;
    unique_ptr<SoldOutState> soldOut_;
};

class NoCoinState : public VendingState {
public:
    Transition insertCoin(VendingMachine &machine, int rupees) override {
        machine.credit(rupees);
        return {StateKind::HasCoin, "accepted, balance Rs " + to_string(machine.balance())};
    }

    Transition selectItem(VendingMachine &) override {
        return {StateKind::NoCoin, "refused: insert a coin first"};
    }

    Transition dispense(VendingMachine &) override {
        return {StateKind::NoCoin, "refused: nothing has been paid for"};
    }

    Transition refund(VendingMachine &) override {
        return {StateKind::NoCoin, "refused: there is no balance to return"};
    }

    Transition refill(VendingMachine &machine, int items) override {
        machine.restock(items);
        return {StateKind::NoCoin, "restocked, " + countOf(machine.items(), "item", "items")};
    }
};

class HasCoinState : public VendingState {
public:
    Transition insertCoin(VendingMachine &machine, int rupees) override {
        machine.credit(rupees);
        return {StateKind::HasCoin, "accepted, balance Rs " + to_string(machine.balance())};
    }

    Transition selectItem(VendingMachine &machine) override {
        if (machine.balance() < machine.price()) {
            return {StateKind::HasCoin,
                    "refused: Rs " + to_string(machine.price() - machine.balance()) + " short"};
        }

        const int change = machine.settle();
        return {StateKind::Dispensing, change > 0 ? "paid, Rs " + to_string(change) + " change"
                                                  : "paid, exact amount"};
    }

    Transition dispense(VendingMachine &) override {
        return {StateKind::HasCoin, "refused: select an item first"};
    }

    Transition refund(VendingMachine &machine) override {
        return {StateKind::NoCoin,
                "returned Rs " + to_string(machine.withdrawBalance()) + ", nothing bought"};
    }

    Transition refill(VendingMachine &, int) override {
        return {StateKind::HasCoin, "refused: finish or refund the payment first"};
    }
};

class DispensingState : public VendingState {
public:
    Transition insertCoin(VendingMachine &, int rupees) override {
        return {StateKind::Dispensing,
                "refused: busy dispensing, Rs " + to_string(rupees) + " returned"};
    }

    Transition selectItem(VendingMachine &) override {
        return {StateKind::Dispensing, "refused: the item is already on its way"};
    }

    Transition dispense(VendingMachine &machine) override {
        machine.releaseItem();

        if (machine.items() == 0) {
            return {StateKind::SoldOut, "item dispensed, the machine is now empty"};
        }
        return {StateKind::NoCoin, "item dispensed, " + countOf(machine.items(), "item", "items") +
                                       " left"};
    }

    Transition refund(VendingMachine &) override {
        return {StateKind::Dispensing, "refused: the payment is already spent"};
    }

    Transition refill(VendingMachine &, int) override {
        return {StateKind::Dispensing, "refused: wait for the dispense to finish"};
    }
};

class SoldOutState : public VendingState {
public:
    Transition insertCoin(VendingMachine &, int rupees) override {
        return {StateKind::SoldOut, "refused: sold out, Rs " + to_string(rupees) + " returned"};
    }

    Transition selectItem(VendingMachine &) override {
        return {StateKind::SoldOut, "refused: sold out"};
    }

    Transition dispense(VendingMachine &) override {
        return {StateKind::SoldOut, "refused: sold out, nothing was paid for"};
    }

    Transition refund(VendingMachine &) override {
        return {StateKind::SoldOut, "refused: nothing to refund"};
    }

    Transition refill(VendingMachine &machine, int items) override {
        machine.restock(items);
        return {StateKind::NoCoin, "restocked, " + countOf(machine.items(), "item", "items")};
    }
};

VendingMachine::VendingMachine(int items, int priceEach)
    : items_(items),
      price_(priceEach),
      noCoin_(make_unique<NoCoinState>()),
      hasCoin_(make_unique<HasCoinState>()),
      dispensing_(make_unique<DispensingState>()),
      soldOut_(make_unique<SoldOutState>()) {
    if (items < 0) {
        throw invalid_argument("vending machine: item count cannot be negative");
    }
    if (priceEach <= 0) {
        throw invalid_argument("vending machine: price must be positive, got " +
                               to_string(priceEach));
    }

    // The initial state is a fact about the machine, decided once here, instead
    // of the first event having to guess it.
    current_ = items_ == 0 ? StateKind::SoldOut : StateKind::NoCoin;
}

VendingMachine::~VendingMachine() = default;

Step VendingMachine::insertCoin(int rupees) {
    // Validated at the boundary so the message can name the caller's argument,
    // and again inside credit() so the machine stays correct regardless of who
    // calls it.
    if (rupees <= 0) {
        throw invalid_argument("insertCoin: a coin must be positive, got " + to_string(rupees));
    }
    return apply(handler(current_)->insertCoin(*this, rupees));
}

Step VendingMachine::selectItem() {
    return apply(handler(current_)->selectItem(*this));
}

Step VendingMachine::dispense() {
    return apply(handler(current_)->dispense(*this));
}

Step VendingMachine::refund() {
    return apply(handler(current_)->refund(*this));
}

Step VendingMachine::refill(int items) {
    if (items <= 0) {
        throw invalid_argument("refill: quantity must be positive, got " + to_string(items));
    }
    return apply(handler(current_)->refill(*this, items));
}

void VendingMachine::credit(int rupees) {
    if (rupees <= 0) {
        throw invalid_argument("credit: a coin must be positive, got " + to_string(rupees));
    }
    balance_ += rupees;
}

// Takes the price and hands back the change, leaving nothing on the machine.
// It empties the balance in one step on purpose: charging the price and leaving
// the overpayment behind is how a machine reports "Rs 20 change" and keeps the
// Rs 20 anyway.
int VendingMachine::settle() {
    if (balance_ < price_) {
        throw logic_error("settle: balance Rs " + to_string(balance_) + " is below the price Rs " +
                          to_string(price_));
    }

    const int change = balance_ - price_;
    balance_ = 0;
    return change;
}

void VendingMachine::releaseItem() {
    if (items_ == 0) {
        throw logic_error("releaseItem: the machine has no items to release");
    }
    --items_;
}

int VendingMachine::withdrawBalance() {
    const int returned = balance_;
    balance_ = 0;
    return returned;
}

void VendingMachine::restock(int items) {
    if (items <= 0) {
        throw invalid_argument("restock: quantity must be positive, got " + to_string(items));
    }
    items_ += items;
}

VendingState *VendingMachine::handler(StateKind kind) const {
    switch (kind) {
        case StateKind::NoCoin:
            return noCoin_.get();
        case StateKind::HasCoin:
            return hasCoin_.get();
        case StateKind::Dispensing:
            return dispensing_.get();
        case StateKind::SoldOut:
            return soldOut_.get();
    }
    throw logic_error("handler: unknown state");
}

Step VendingMachine::apply(Transition transition) {
    Step step{current_, transition.next, std::move(transition.message)};
    current_ = transition.next;
    return step;
}

namespace {

constexpr int ITEM_PRICE_RUPEES = 20;
constexpr int INITIAL_ITEMS = 2;

void report(int number, const string &action, const Step &step) {
    cout << "\n" << number << ". " << action << "\n";
    cout << "   " << step.message << "\n";
    cout << "   " << name(step.from) << " -> " << name(step.to) << "\n";
}

void printStatus(const VendingMachine &machine) {
    cout << "   [" << name(machine.state()) << "]  "
         << countOf(machine.items(), "item", "items") << " · Rs " << machine.balance()
         << " inserted · price Rs " << machine.price() << "\n";
}

}

int main() {
    cout << "State\n\n";

    VendingMachine machine(INITIAL_ITEMS, ITEM_PRICE_RUPEES);
    cout << "Water bottles at Rs " << ITEM_PRICE_RUPEES << " each, " << INITIAL_ITEMS
         << " in stock\n";
    printStatus(machine);

    int number = 1;

    cout << "\n--- one purchase, with a wrong turn at every step ---";
    report(number++, "selectItem() before any coin", machine.selectItem());
    report(number++, "insertCoin(10)", machine.insertCoin(10));
    report(number++, "dispense() before selecting", machine.dispense());
    report(number++, "selectItem() with only Rs 10", machine.selectItem());
    report(number++, "insertCoin(10)", machine.insertCoin(10));
    report(number++, "selectItem() with Rs 20", machine.selectItem());

    cout << "\n--- the machine is busy, so nothing else lands ---";
    report(number++, "selectItem() while dispensing", machine.selectItem());
    report(number++, "refill(2) while dispensing", machine.refill(2));
    report(number++, "insertCoin(5) while dispensing", machine.insertCoin(5));
    report(number++, "refund() while dispensing", machine.refund());
    report(number++, "dispense()", machine.dispense());
    printStatus(machine);

    cout << "\n--- take the money back ---";
    report(number++, "refund() with no balance", machine.refund());
    report(number++, "insertCoin(20)", machine.insertCoin(20));
    report(number++, "refund() with Rs 20 in", machine.refund());
    printStatus(machine);

    cout << "\n--- overpay, and empty the machine ---";
    report(number++, "insertCoin(20)", machine.insertCoin(20));
    report(number++, "insertCoin(20) again", machine.insertCoin(20));
    report(number++, "selectItem() with Rs 40", machine.selectItem());
    report(number++, "dispense() of the last bottle", machine.dispense());
    printStatus(machine);

    cout << "\n--- sold out: every door is closed ---";
    report(number++, "insertCoin(20)", machine.insertCoin(20));
    report(number++, "selectItem()", machine.selectItem());
    report(number++, "dispense()", machine.dispense());
    report(number++, "refund()", machine.refund());
    report(number++, "refill(2)", machine.refill(2));
    printStatus(machine);

    cout << "\n--- refilling while a coin is in ---";
    report(number++, "insertCoin(20)", machine.insertCoin(20));
    report(number++, "refill(2) with Rs 20 in", machine.refill(2));
    report(number++, "refund()", machine.refund());
    report(number++, "refill(2)", machine.refill(2));
    printStatus(machine);

    cout << "\n--- input that no state should ever see ---\n";
    try {
        machine.insertCoin(0);
        cout << "   accepted (should not happen)\n";
    } catch (const invalid_argument &error) {
        cout << "   0 rejected: " << error.what() << "\n";
    }
    try {
        machine.refill(-1);
        cout << "   accepted (should not happen)\n";
    } catch (const invalid_argument &error) {
        cout << "   -1 rejected: " << error.what() << "\n";
    }

    cout << "\n--- a machine stocked with nothing starts sold out ---\n";
    VendingMachine empty(0, ITEM_PRICE_RUPEES);
    printStatus(empty);

    cout << "\nThe machine never asked which state it was in; every event was\n"
            "answered by whichever state was current, and that state said where\n"
            "to go next. A sixth state is a class, a line in the constructor and\n"
            "one case in handler() - no event method changes.\n";

    return 0;
}
