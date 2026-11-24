#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

// MEDIATOR, part 2 of 2 - one object owns the conversation.
//
// A User knows the room and nothing else. There is no peer list, so there is
// nothing to wire and nothing to wire wrong: joining is one call, and a user
// cannot be half-connected. All routing lives in ChatRoom -
//
//     broadcast   every member except the sender
//     direct      one member, addressed by name
//     mute        "do not deliver this speaker's messages to this listener"
//
// - which makes a rule that involves two people a single check in deliver()
// instead of a copy in every send path.
//
// Ownership. The room owns the users and is the only thing that can create
// one, so a member cannot exist without joining. Leaving is one erase and
// cannot leave anyone holding a pointer to a freed user; the flip side is that
// a reference handed out by join() dies with the user it points at.
//
// Delivery. User::receive is private and ChatRoom is the only friend. That is
// the spoofing fix from part 1 - there is no public way to claim a sender, and
// the compiler enforces it rather than a comment in the manual.
//
// The cost, stated honestly: the room now knows every user, and every new kind
// of interaction is a new method on it. The mesh's N*(N-1) links become N
// references plus one object everybody depends on.

class ChatRoom;

class User {
public:
    const string &name() const {
        return name_;
    }

    void send(const string &message) const;
    void sendTo(const string &to, const string &message) const;

    // "Stop delivering `who`'s messages to me."
    void mute(const string &who) const;
    bool unmute(const string &who) const;

private:
    friend class ChatRoom;

    // Only the room may create a member, so a User that never joined cannot
    // exist and cannot speak.
    User(string name, ChatRoom &room) : name_(std::move(name)), room_(room) {}

    // Only the room may hand a user a message.
    void receive(const string &from, const string &message) const {
        cout << "    " << name_ << " got from " << from << ": " << message << "\n";
    }

    string name_;
    ChatRoom &room_;
};

class ChatRoom {
public:
    // The room validates the name, so a member always has one identity and a
    // private message can be addressed by it.
    const User &join(string name);

    // Drops the member and every mute rule that names them, in both directions.
    bool leave(const string &name);

    void broadcast(const User &from, const string &message) const;

    // Refuses an unknown recipient rather than silently dropping the message.
    bool direct(const User &from, const string &to, const string &message) const;

    void mute(const string &listener, const string &speaker);
    bool unmute(const string &listener, const string &speaker);

    bool isMuted(const string &listener, const string &speaker) const;

    size_t memberCount() const {
        return users_.size();
    }

private:
    void deliver(const User &to, const User &from, const string &message) const;

    map<string, User> users_;
    map<string, set<string>> mutes_;
};

void User::send(const string &message) const {
    room_.broadcast(*this, message);
}

void User::sendTo(const string &to, const string &message) const {
    room_.direct(*this, to, message);
}

void User::mute(const string &who) const {
    room_.mute(name_, who);
}

bool User::unmute(const string &who) const {
    return room_.unmute(name_, who);
}

const User &ChatRoom::join(string name) {
    if (name.empty()) {
        throw invalid_argument("join: a name is required");
    }

    const auto inserted = users_.emplace(name, User(name, *this));
    if (!inserted.second) {
        throw invalid_argument("join: " + name + " is already in the room");
    }
    return inserted.first->second;
}

bool ChatRoom::leave(const string &name) {
    if (users_.erase(name) == 0) {
        return false;
    }

    // A rule naming someone who is gone can never fire again, so it goes too.
    mutes_.erase(name);
    for (auto &entry : mutes_) {
        entry.second.erase(name);
    }
    return true;
}

void ChatRoom::broadcast(const User &from, const string &message) const {
    cout << "[" << from.name() << " broadcasts] " << message << "\n";
    for (const auto &entry : users_) {
        if (entry.first == from.name()) {
            continue;
        }
        deliver(entry.second, from, message);
    }
}

bool ChatRoom::direct(const User &from, const string &to, const string &message) const {
    if (to == from.name()) {
        cout << "[" << from.name() << " -> " << to << "] refused: cannot message yourself\n";
        return false;
    }

    const auto found = users_.find(to);
    if (found == users_.end()) {
        cout << "[" << from.name() << " -> " << to << "] refused: no such member\n";
        return false;
    }

    cout << "[" << from.name() << " -> " << to << "] " << message << "\n";
    deliver(found->second, from, message);
    return true;
}

void ChatRoom::mute(const string &listener, const string &speaker) {
    if (listener == speaker) {
        throw invalid_argument("mute: " + listener + " cannot mute themselves");
    }
    if (users_.find(listener) == users_.end()) {
        throw invalid_argument("mute: unknown member " + listener);
    }
    if (users_.find(speaker) == users_.end()) {
        throw invalid_argument("mute: unknown member " + speaker);
    }

    mutes_[listener].insert(speaker);
}

bool ChatRoom::unmute(const string &listener, const string &speaker) {
    const auto found = mutes_.find(listener);
    if (found == mutes_.end()) {
        return false;
    }

    const bool removed = found->second.erase(speaker) != 0;
    if (found->second.empty()) {
        mutes_.erase(found);
    }
    return removed;
}

bool ChatRoom::isMuted(const string &listener, const string &speaker) const {
    const auto found = mutes_.find(listener);
    return found != mutes_.end() && found->second.count(speaker) != 0;
}

void ChatRoom::deliver(const User &to, const User &from, const string &message) const {
    if (isMuted(to.name(), from.name())) {
        cout << "    (skipped " << to.name() << " - muted " << from.name() << ")\n";
        return;
    }
    to.receive(from.name(), message);
}

int main() {
    cout << "With Mediator - one room owns who hears whom\n";

    ChatRoom room;
    const User &rohan = room.join("Rohan");
    const User &neha = room.join("Neha");
    const User &mohan = room.join("Mohan");

    cout << "\n1. Three users joined\n";
    cout << "   3 join() calls, 0 pointers written between users\n";
    cout << "   members: " << room.memberCount() << "\n";

    cout << "\n2. Broadcast reaches every member except the sender\n";
    rohan.send("Hello everyone!");

    cout << "\n3. Private messages, addressed by name\n";
    mohan.sendTo("Neha", "Hey Neha!");
    cout << "   talking to someone who is not here:\n";
    mohan.sendTo("Priya", "Anyone there?");
    cout << "   talking to yourself:\n";
    mohan.sendTo("Mohan", "Note to self");

    cout << "\n4. Mute is one rule, checked where delivery happens\n";
    rohan.mute("Mohan");
    mohan.send("Rohan should not see this");

    cout << "\n5. A second, independent rule, then removing it\n";
    neha.mute("Mohan");
    mohan.send("nobody hears this one");
    neha.unmute("Mohan");
    mohan.send("Neha hears this again, Rohan still does not");

    cout << "\n6. A fourth member costs one call and no pointer writes\n";
    room.join("Priya");
    cout << "   members: " << room.memberCount() << ", each holding one reference to the room\n";
    rohan.send("Priya joins and just works");

    cout << "\n7. A member leaves, and the room takes their rules with them\n";
    room.leave("Mohan");
    cout << "   members: " << room.memberCount() << "\n";
    cout << "   Rohan's rule against Mohan still there? "
         << (room.isMuted("Rohan", "Mohan") ? "yes" : "no, the room erased it") << "\n";
    rohan.send("Mohan is gone, so the other two hear this");

    cout << "\nFour join() calls, one broadcast path, one mute check, one place\n"
            "that can erase a member. The fourth member was one join() and\n"
            "nothing else; the mesh needed three more hand-written pairings.\n"
            "\n"
            "The spoof from part 1 does not compile here: receive() and the User\n"
            "constructor are private, ChatRoom is their only friend, so nothing\n"
            "can claim a sender or invent a member.\n"
            "\n"
            "That guarantee is not free. The room knows every member, and every new\n"
            "interaction is another method on it - the pattern trades N*(N-1) links\n"
            "for N references and one object everyone depends on.\n";

    return 0;
}
