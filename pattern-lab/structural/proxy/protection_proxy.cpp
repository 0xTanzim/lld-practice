#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
using namespace std;

class IDocumentReader {
public:
    virtual ~IDocumentReader() = default;
    virtual bool unlockPDF(const string &filePath, const string &password) = 0;
};

class RealDocumentReader : public IDocumentReader {
public:
    bool unlockPDF(const string &filePath, const string &password) override {
        (void)password;
        cout << "[RealDocumentReader] Unlocking PDF at: " << filePath << "\n";
        cout << "[RealDocumentReader] PDF unlocked successfully (password: ****)\n";
        cout << "[RealDocumentReader] Displaying PDF content...\n";
        return true;
    }
};

class User {
private:
    string name;
    bool premiumMembership;

public:
    User(const string &name, bool isPremium) : name(name), premiumMembership(isPremium) {}

    const string &getName() const {
        return name;
    }

    bool isPremium() const {
        return premiumMembership;
    }
};

class DocumentProxy : public IDocumentReader {
private:
    const User *user;
    unique_ptr<RealDocumentReader> realReader;

public:
    DocumentProxy(const User *user) : user(user), realReader(make_unique<RealDocumentReader>()) {
        if (!user) {
            throw invalid_argument("DocumentProxy requires a user.");
        }
    }

    bool unlockPDF(const string &filePath, const string &password) override {
        if (!user->isPremium()) {
            cout << "[DocumentProxy] Access denied for " << user->getName()
                 << ". Only premium members can unlock PDFs.\n";
            return false;
        }
        return realReader->unlockPDF(filePath, password);
    }
};

int main() {
    User rohan("Rohan", false);
    User rashmi("Rashmi", true);

    cout << "== Rohan (Non-Premium) tries to unlock PDF ==\n";
    DocumentProxy reader1(&rohan);
    bool ok1 = reader1.unlockPDF("protected_document.pdf", "secret123");
    cout << "Result: " << (ok1 ? "unlocked" : "denied") << "\n";

    cout << "\n== Rashmi (Premium) unlocks PDF ==\n";
    DocumentProxy reader2(&rashmi);
    bool ok2 = reader2.unlockPDF("protected_document.pdf", "secret123");
    cout << "Result: " << (ok2 ? "unlocked" : "denied") << "\n";

    return 0;
}
