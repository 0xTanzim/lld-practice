#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// MEDIATOR, part 1 of 2 - every participant holds a pointer to every other one.
//
// The mesh is paid for twice. Once when it is built: N people need N*(N-1)/2
// hand-written pairings, and nothing checks that both directions were written.
// And once per rule: "do not deliver to someone who muted me" has no owner, so
// whoever delivers has to reach into the receiver's public mute list and apply
// the rule itself. Anything that skips that object skips the rule.
//
// Two failures are demonstrated below, and neither is a compile error:
//
//   - a one-way link. The person is in the chat, broadcasts, and simply never
//     reaches the other side.
//   - a delivery straight into receive(). It is public, so any object can hand
//     a message to anyone, claim it came from anyone, and walk past the mute
//     rule on the way in.
//
// 02_with_mediator.cpp keeps the same users, the same messages and the same
// mute, and replaces the mesh with one object that owns the topology.

class User {
public:
    explicit User(string name) : name_(std::move(name)) {}

    const string &name() const {
        return name_;
    }

    // One arrow, and both arrows are the caller's problem. Nothing here can
    // tell whether the pair was completed.
    void addPeer(User &peer) {
        peers_.push_back(&peer);
    }

    // "Stop delivering messages from `who` to me."
    void mute(const string &who) {
        muted_.insert(who);
    }

    bool isMuted(const string &who) const {
        return muted_.count(who) != 0;
    }

    void send(const string &message) const {
        cout << "[" << name_ << " broadcasts] " << message << "\n";
        for (const User *peer : peers_) {
            deliverTo(*peer, message);
        }
    }

    void sendTo(const User &target, const string &message) const {
        cout << "[" << name_ << " -> " << target.name_ << "] " << message << "\n";
        deliverTo(target, message);
    }

    // Public, and it believes whatever sender it is told.
    void receive(const string &from, const string &message) const {
        cout << "    " << name_ << " got from " << from << ": " << message << "\n";
    }

    size_t linkCount() const {
        return peers_.size();
    }

private:
    // The sender enforces the receiver's mute list, because the receiver's list
    // is the only place the rule exists.
    void deliverTo(const User &target, const string &message) const {
        if (target.isMuted(name_)) {
            cout << "    (skipped " << target.name_ << " - muted " << name_ << ")\n";
            return;
        }
        target.receive(name_, message);
    }

    string name_;
    vector<User *> peers_;
    set<string> muted_;
};

namespace {

// Both arrows, by hand. Forgetting the second call is the failure in step 5.
void link(User &first, User &second) {
    first.addPeer(second);
    second.addPeer(first);
}

size_t totalLinks(const vector<const User *> &users) {
    size_t total = 0;
    for (const User *user : users) {
        total += user->linkCount();
    }
    return total;
}

}

int main() {
    cout << "Without Mediator - every user holds a pointer to every other user\n";

    // The original allocated these with new and deleted them by hand at the end
    // of main. Ownership is not what this file is about, so they are automatic
    // objects and the defect under the microscope stays the only one.
    User rohan("Rohan");
    User neha("Neha");
    User mohan("Mohan");

    cout << "\n1. Wiring three users\n";
    link(rohan, neha);
    link(rohan, mohan);
    link(neha, mohan);
    cout << "   3 link() calls, each writing 2 pointers\n";
    cout << "   " << totalLinks({&rohan, &neha, &mohan}) << " pointers held for 3 users\n";

    cout << "\n2. Broadcast\n";
    rohan.send("Hello everyone!");

    cout << "\n3. Muting is directional, and the sender has to honour it\n";
    rohan.mute("Mohan");
    cout << "   Rohan mutes Mohan, then Rohan speaks - nobody is skipped, because\n"
            "   the rule only filters what Rohan receives:\n";
    rohan.send("this still reaches Mohan");
    cout << "   now Mohan speaks:\n";
    mohan.send("this one never reaches Rohan");

    cout << "\n4. A fourth user costs six more pointers\n";
    User priya("Priya");
    link(rohan, priya);
    link(neha, priya);
    link(mohan, priya);
    cout << "   3 more link() calls\n";
    cout << "   " << totalLinks({&rohan, &neha, &mohan, &priya})
         << " pointers held for 4 users (n*(n-1))\n";

    cout << "\n5. One direction forgotten, and the chat is half duplex\n";
    User kabir("Kabir");
    rohan.addPeer(kabir);  // Rohan can reach Kabir, but not the other way round.
    rohan.send("Kabir hears this");
    kabir.send("nothing happens here");
    cout << "   Kabir's peer list holds " << kabir.linkCount() << " entries\n";

    cout << "\n6. receive() is public, so the message goes straight in\n";
    neha.receive("Mohan", "Mohan never sent this");
    rohan.receive("Mohan", "Rohan muted Mohan, and it still arrives");

    cout << "\nNothing owns the route between two people: the sender applies the\n"
            "receiver's mute list by hand, a public receive() walks past it, and\n"
            "adding someone means editing every other list. Part 2 moves the\n"
            "topology into a single object.\n";

    return 0;
}
