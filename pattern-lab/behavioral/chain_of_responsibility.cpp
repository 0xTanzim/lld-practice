#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace std;

class MoneyHandler {
protected:
    MoneyHandler *nextHandler = nullptr;

public:
    virtual ~MoneyHandler() = default;
    void setNextHandler(MoneyHandler *next) {
        nextHandler = next;
    }
    virtual void dispense(int amount) = 0;
};

class NoteHandler : public MoneyHandler {
private:
    int denomination;
    int numNotes;

public:
    NoteHandler(int denomination, int numNotes)
        : denomination(denomination), numNotes(numNotes) {
        if (denomination <= 0 || numNotes < 0) {
            throw invalid_argument("Denomination must be positive and note count non-negative.");
        }
    }

    void dispense(int amount) override {
        if (amount <= 0) {
            return;
        }

        int notesNeeded = min(amount / denomination, numNotes);
        numNotes -= notesNeeded;
        if (notesNeeded > 0) {
            cout << "Dispensing " << notesNeeded << " x $" << denomination << " notes.\n";
        }

        int remainingAmount = amount - (notesNeeded * denomination);
        if (remainingAmount > 0) {
            if (nextHandler != nullptr) {
                nextHandler->dispense(remainingAmount);
            } else {
                cout << "Remaining amount of $" << remainingAmount
                     << " cannot be fulfilled (insufficient funds in ATM)\n";
            }
        }
    }
};

int main() {
    NoteHandler hundred(100, 3);
    NoteHandler fifty(50, 5);
    NoteHandler twenty(20, 10);
    NoteHandler ten(10, 20);

    hundred.setNextHandler(&fifty);
    fifty.setNextHandler(&twenty);
    twenty.setNextHandler(&ten);

    cout << "\nDispensing amount: $400\n";
    hundred.dispense(400);

    cout << "\nDispensing amount: $1000\n";
    hundred.dispense(1000);

    return 0;
}
