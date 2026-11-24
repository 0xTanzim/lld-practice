#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// MEMENTO - save an object's state so it can be restored later, without letting
// the object that holds the save read it.
//
// Three roles, and the third one is the point:
//
//   originator  Database            owns the records, makes and applies saves
//   memento     Snapshot            the saved state, opaque to everyone except
//                                   the database that produced it
//   caretaker   TransactionManager  keeps the saves, never looks inside
//
// The version this came from deleted raw memento pointers in three separate
// places, exposed the saved records through a public getState(), accepted a
// restore from a snapshot belonging to a different database, and let begin()
// quietly overwrite an open backup - so a nested transaction threw away the
// outer rollback point. Those four are what this file fixes.

class Database;

// The memento. No public constructor and no accessor for the records: the
// caretaker can hold it, pass it back, and ask how big it is - nothing more.
class Snapshot {
public:
    const string &label() const {
        return label_;
    }

    size_t recordCount() const {
        return records_.size();
    }

private:
    friend class Database;

    Snapshot(const Database &owner, string label, map<string, string> records);

    const unsigned ownerId_;
    const string label_;
    const map<string, string> records_;
};

namespace {

string countOf(size_t count, const string &noun) {
    return to_string(count) + " " + noun + (count == 1 ? "" : "s");
}

}

class Database {
public:
    explicit Database(string name) : id_(nextId()), name_(std::move(name)) {
        if (name_.empty()) {
            throw invalid_argument("Database: name must not be empty");
        }
    }

    // A copy would duplicate both the identity and the records, and the
    // snapshot check below relies on the identity being unique.
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    unsigned id() const {
        return id_;
    }

    const string &name() const {
        return name_;
    }

    // Insert, update and erase each refuse the case they cannot serve instead
    // of printing a note and carrying on.
    void insert(string key, string value) {
        checkKey(key);
        if (records_.find(key) != records_.end()) {
            throw invalid_argument("Database: record \"" + key + "\" already exists");
        }
        records_.emplace(std::move(key), std::move(value));
    }

    void update(const string &key, string value) {
        const auto found = records_.find(key);
        if (found == records_.end()) {
            throw out_of_range("Database: no record \"" + key + "\" to update");
        }
        found->second = std::move(value);
    }

    void erase(const string &key) {
        if (records_.erase(key) == 0) {
            throw out_of_range("Database: no record \"" + key + "\" to erase");
        }
    }

    unique_ptr<Snapshot> snapshot(string label) const {
        if (label.empty()) {
            throw invalid_argument("Database: a snapshot needs a label");
        }
        // Constructed here, in the originator's scope, because the memento's
        // constructor is private. make_unique would be rejected: its own body
        // has no access to that constructor.
        return unique_ptr<Snapshot>(new Snapshot(*this, std::move(label), records_));
    }

    void restore(const Snapshot &snapshot) {
        if (snapshot.ownerId_ != id_) {
            throw invalid_argument("Database: snapshot \"" + snapshot.label_ +
                                   "\" belongs to database #" + to_string(snapshot.ownerId_));
        }
        records_ = snapshot.records_;
    }

    const map<string, string> &records() const {
        return records_;
    }

    size_t size() const {
        return records_.size();
    }

private:
    static unsigned nextId() {
        static unsigned last = 0;
        return ++last;
    }

    static void checkKey(const string &key) {
        if (key.empty()) {
            throw invalid_argument("Database: key must not be empty");
        }
    }

    const unsigned id_;
    const string name_;
    map<string, string> records_;
};

Snapshot::Snapshot(const Database &owner, string label, map<string, string> records)
    : ownerId_(owner.id()), label_(std::move(label)), records_(std::move(records)) {}

// The caretaker. It owns the snapshots, never reads them, and keeps one per
// open transaction so a nested one rolls back to its own starting point rather
// than to whatever was saved last.
class TransactionManager {
public:
    // Returns the depth: 1 for the outermost transaction, 2 for a nested one.
    size_t begin(Database &db, string label) {
        stack_.push_back(db.snapshot(std::move(label)));
        return stack_.size();
    }

    // Throws away the innermost snapshot and keeps the changes it covered.
    string commit() {
        requireOpen("commit");
        const string label = stack_.back()->label();
        stack_.pop_back();
        return label;
    }

    // Restores the innermost snapshot, then drops it. restore() throws before
    // anything is popped if the snapshot belongs to a different database.
    string rollback(Database &db) {
        requireOpen("rollback");
        db.restore(*stack_.back());
        const string label = stack_.back()->label();
        stack_.pop_back();
        return label;
    }

    size_t depth() const {
        return stack_.size();
    }

    // How much state is being held - never which state.
    size_t recordCount() const {
        return stack_.empty() ? 0 : stack_.back()->recordCount();
    }

private:
    void requireOpen(const string &operation) const {
        if (stack_.empty()) {
            throw logic_error("TransactionManager: " + operation + " with no transaction open");
        }
    }

    vector<unique_ptr<Snapshot>> stack_;
};

namespace {

void printRecords(const Database &db, const string &label) {
    cout << "   " << label << " — " << countOf(db.size(), "record") << "\n";
    if (db.records().empty()) {
        cout << "     (empty)\n";
        return;
    }
    for (const auto &[key, value] : db.records()) {
        cout << "     " << key << " = " << value << "\n";
    }
}

}

int main() {
    cout << "Memento - snapshot a database, roll back what went wrong\n";

    Database db("users");
    db.insert("user1", "Aditya");
    db.insert("user2", "Rohit");
    TransactionManager transactions;

    cout << "\n1. Starting point\n";
    printRecords(db, "users");

    cout << "\n2. A change that succeeds: snapshot, edit, commit\n";
    cout << "   begin \"add-users\" (depth " << transactions.begin(db, "add-users") << ")\n";
    cout << "   the caretaker holds " << countOf(transactions.recordCount(), "record")
         << " and cannot read any of them\n";
    cout << "   insert user3 = Saurav\n";
    db.insert("user3", "Saurav");
    cout << "   commit \"" << transactions.commit() << "\" — snapshot discarded, change kept\n";
    printRecords(db, "users");

    cout << "\n3. A change that fails: snapshot, edit, roll back\n";
    cout << "   begin \"bad-batch\" (depth " << transactions.begin(db, "bad-batch") << ")\n";
    cout << "   insert user4 = Manish\n";
    db.insert("user4", "Manish");
    cout << "   insert user5 = Kabir\n";
    db.insert("user5", "Kabir");
    printRecords(db, "before rollback");
    cout << "   rollback \"" << transactions.rollback(db) << "\" — both inserts undone\n";
    printRecords(db, "users");

    cout << "\n4. Nested: each level keeps its own rollback point\n";
    cout << "   begin \"outer\" (depth " << transactions.begin(db, "outer") << ")\n";
    cout << "   insert user6 = Priya\n";
    db.insert("user6", "Priya");
    cout << "   begin \"inner\" (depth " << transactions.begin(db, "inner") << ")\n";
    cout << "   insert user7 = Neha\n";
    db.insert("user7", "Neha");
    cout << "   rollback \"" << transactions.rollback(db) << "\" — user7 dropped, user6 kept\n";
    printRecords(db, "users");
    cout << "   rollback \"" << transactions.rollback(db) << "\" — user6 dropped too\n";
    printRecords(db, "users");
    cout << "   depth is now " << transactions.depth() << "\n";

    cout << "\n5. What it refuses\n";
    try {
        transactions.commit();
    } catch (const logic_error &error) {
        cout << "   refused: " << error.what() << "\n";
    }

    try {
        db.insert("user1", "Someone else");
    } catch (const invalid_argument &error) {
        cout << "   refused: " << error.what() << "\n";
    }

    try {
        db.update("user99", "Nobody");
    } catch (const out_of_range &error) {
        cout << "   refused: " << error.what() << "\n";
    }

    Database analytics("analytics");
    analytics.insert("page1", "views");
    const unique_ptr<Snapshot> foreign = analytics.snapshot("analytics-today");
    try {
        db.restore(*foreign);
    } catch (const invalid_argument &error) {
        cout << "   refused: " << error.what() << "\n";
    }
    cout << "   ... and the caretaker still only knows \"" << foreign->label() << "\" holds "
         << countOf(foreign->recordCount(), "record") << "\n";

    cout << "\nA snapshot is a full copy, so it costs one record per record and the\n"
            "memory grows with the database. That is the trade: restore is cheap and\n"
            "exact, the saved state is expensive, and the caretaker can hold it\n"
            "without being able to read it or change it.\n";

    return 0;
}
