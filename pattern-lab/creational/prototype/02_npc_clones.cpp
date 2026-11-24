#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

// PROTOTYPE, part 2 of 2 - the other reason to clone: construction is expensive.
//
// Part 1 clones so the caller does not have to name a concrete class. This one
// clones so the caller does not have to pay for construction again. The
// template NPC is built once - a database lookup and a stat roll in the version
// this came from, a counter and one line of output here - and every variant
// after it is a copy.
//
// Four things in the original were real faults, not style:
//
//   Cloneable* clone()   a raw pointer. Every caller had to remember delete,
//                        and the interface gave no way to fail safe.
//   dynamic_cast<NPC*>   clone() already returns the right type. The cast was
//                        there to prove it, and on a wrong assumption it yields
//                        nullptr - which the next line dereferences.
//   four setters         setName/setHealth/setAttack/setDefense wrote fields
//                        with no rules: an NPC could be renamed to "", given
//                        0 HP, or have 40 attack taken away.
//   describe()           not const, so an NPC that must not change could not
//                        print itself.
//
// There is no Cloneable base here. With one class it would only add a virtual
// call and the cast that follows from it; part 1 keeps the interface because
// there the caller genuinely does not know which campaign it gets.

struct Stats {
    int health;
    int attack;
    int defense;

    // One place decides what a legal stat block is. A fresh NPC and a trained
    // clone both pass through it, so the same rules hold either way.
    Stats(int health, int attack, int defense)
        : health(health), attack(attack), defense(defense) {
        if (health <= 0) {
            throw invalid_argument("Stats: health must be positive");
        }
        if (attack < 0) {
            throw invalid_argument("Stats: attack must not be negative");
        }
        if (defense < 0) {
            throw invalid_argument("Stats: defense must not be negative");
        }
    }
};

class Npc {
public:
    // The expensive path: lookup, calculation, and in the real thing a database
    // round trip. Counted so the demo can show it ran once.
    Npc(string name, Stats stats) : name_(std::move(name)), stats_(stats) {
        checkName(name_);
        ++builds_;
        cout << "  built \"" << name_ << "\" from scratch\n";
    }

    // The cheap path. clone() lands here, and so does any other copy.
    Npc(const Npc &other) : name_(other.name_), stats_(other.stats_) {
        ++clones_;
        cout << "  copied \"" << name_ << "\" (copy " << clones_ << ")\n";
    }

    // Prototype: the object reproduces itself and the caller owns the result.
    unique_ptr<Npc> clone() const {
        return make_unique<Npc>(*this);
    }

    void rename(string value) {
        checkName(value);
        name_ = std::move(value);
    }

    // A tweak, applied to the clone only: deltas in, validated by Stats, so no
    // variant ends up below 1 HP or with negative attack.
    void train(int health, int attack, int defense) {
        stats_ = Stats(stats_.health + health, stats_.attack + attack,
                       stats_.defense + defense);
    }

    const string &name() const {
        return name_;
    }

    const Stats &stats() const {
        return stats_;
    }

    void describe(const string &label) const {
        cout << "  " << label << ": " << name_ << " [HP=" << stats_.health
             << " ATK=" << stats_.attack << " DEF=" << stats_.defense << "]\n";
    }

    static int builds() {
        return builds_;
    }

    static int clones() {
        return clones_;
    }

private:
    static void checkName(const string &value) {
        if (value.empty()) {
            throw invalid_argument("Npc: name must not be empty");
        }
    }

    string name_;
    Stats stats_;

    inline static int builds_ = 0;
    inline static int clones_ = 0;
};

int main() {
    cout << "Prototype, part 2 - build once, clone the variants\n";

    cout << "\n1. The template, built once\n";
    Npc alien("Alien", Stats(30, 5, 2));
    cout << "  builds: " << Npc::builds() << ", copies: " << Npc::clones() << "\n";

    cout << "\n2. Three variants, none of them built\n";
    // No new, no delete, no cast: each clone owns itself and knows its type.
    unique_ptr<Npc> scout = alien.clone();
    scout->rename("Alien Scout");

    unique_ptr<Npc> brute = alien.clone();
    brute->rename("Alien Brute");
    brute->train(20, 3, 1);

    unique_ptr<Npc> queen = alien.clone();
    queen->rename("Alien Queen");
    queen->train(40, 8, 6);

    alien.describe("template");
    scout->describe("variant A");
    brute->describe("variant B");
    queen->describe("variant C");
    cout << "  builds: " << Npc::builds() << ", copies: " << Npc::clones()
         << " - building the three variants instead would have been 4 builds\n";

    cout << "\n3. Each clone owns its own stats\n";
    brute->train(0, 0, 25);
    brute->describe("variant B tweaked");
    alien.describe("template");
    cout << "  ...the template's DEF did not move\n";

    cout << "\n4. A tweak that would break an NPC is refused\n";
    try {
        scout->train(-40, 0, 0);
    } catch (const invalid_argument &error) {
        cout << "  refused: " << error.what() << "\n";
    }
    try {
        queen->rename("");
    } catch (const invalid_argument &error) {
        cout << "  refused: " << error.what() << "\n";
    }

    cout << "\n5. What it costs, what it saves\n"
            "  1 construction + "
         << Npc::clones()
         << " copies for 4 NPCs, each still validating its own stats.\n\n"
            "  Part 1 is the same clone when the class name is unknown. This is\n"
            "  the reason to reach for it when the class name is the one thing\n"
            "  you do know, and building it is the expensive part.\n";

    return 0;
}
