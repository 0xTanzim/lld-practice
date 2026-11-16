#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// PROTOTYPE: clone an existing configured object instead of naming its class
// and rebuilding it.
//
// The problem it solves: "copy this campaign" through a base-class pointer.
// The copy constructor cannot do that - it is not virtual, so it slices the
// derived part, and the caller has to know the concrete type to call it at all.
// A virtual clone() answers both: the object copies itself, at its real type.
//
// It is also a deep copy. The channels live inside the campaign, not behind a
// shared pointer, so the copy owns its own budget figures. Editing the clone
// must never move the template.

// One channel's settings inside a campaign. Value type on purpose: any
// prototype clone copies these, and no two campaigns can end up sharing one.
struct Channel {
    string name;
    int budget;
};

class Campaign {
public:
    virtual ~Campaign() = default;

    // The prototype method. Each concrete campaign returns its own type.
    virtual unique_ptr<Campaign> clone() const = 0;

    virtual string kind() const = 0;

    const string &name() const {
        return name_;
    }

    void rename(string value) {
        if (value.empty()) {
            throw invalid_argument("Campaign: name must not be empty");
        }
        name_ = std::move(value);
    }

    void setBudget(size_t index, int amount) {
        if (index >= channels_.size()) {
            throw out_of_range("Campaign: no channel at index " + to_string(index));
        }
        if (amount < 0) {
            throw invalid_argument("Campaign: budget must not be negative");
        }
        channels_[index].budget = amount;
    }

    const vector<Channel> &channels() const {
        return channels_;
    }

    int totalBudget() const {
        int total = 0;
        for (const Channel &channel : channels_) {
            total += channel.budget;
        }
        return total;
    }

protected:
    Campaign(string name, vector<Channel> channels)
        : name_(std::move(name)), channels_(std::move(channels)) {
        if (name_.empty()) {
            throw invalid_argument("Campaign: name must not be empty");
        }
        if (channels_.empty()) {
            throw invalid_argument("Campaign: a campaign needs at least one channel");
        }
    }

    string name_;
    vector<Channel> channels_;
};

class FlashSaleCampaign : public Campaign {
public:
    FlashSaleCampaign(string name, vector<Channel> channels, int hours)
        : Campaign(std::move(name), std::move(channels)), hours_(hours) {
        if (hours_ <= 0) {
            throw invalid_argument("FlashSaleCampaign: hours must be positive");
        }
    }

    // The copy constructor does the work: base part + vector of channel values.
    unique_ptr<Campaign> clone() const override {
        return make_unique<FlashSaleCampaign>(*this);
    }

    string kind() const override {
        return "flash sale (" + to_string(hours_) + " h)";
    }

private:
    int hours_;
};

class WelcomeCampaign : public Campaign {
public:
    WelcomeCampaign(string name, vector<Channel> channels, string audience)
        : Campaign(std::move(name), std::move(channels)), audience_(std::move(audience)) {
        if (audience_.empty()) {
            throw invalid_argument("WelcomeCampaign: audience must not be empty");
        }
    }

    unique_ptr<Campaign> clone() const override {
        return make_unique<WelcomeCampaign>(*this);
    }

    string kind() const override {
        return "welcome (" + audience_ + ")";
    }

private:
    string audience_;
};

// The second half of the pattern: a registry of configured prototypes. Callers
// ask for a name and receive a fresh copy - never the template itself, so a
// caller cannot corrupt the next caller's starting point.
class CampaignRegistry {
public:
    void add(const string &key, unique_ptr<Campaign> prototype) {
        if (key.empty()) {
            throw invalid_argument("CampaignRegistry: key must not be empty");
        }
        if (prototypes_.find(key) != prototypes_.end()) {
            throw invalid_argument("CampaignRegistry: duplicate key " + key);
        }
        prototypes_.emplace(key, std::move(prototype));
    }

    unique_ptr<Campaign> create(const string &key) const {
        const auto found = prototypes_.find(key);
        if (found == prototypes_.end()) {
            throw out_of_range("CampaignRegistry: unknown template " + key);
        }
        return found->second->clone();
    }

    size_t size() const {
        return prototypes_.size();
    }

private:
    map<string, unique_ptr<Campaign>> prototypes_;
};

void printCampaign(const Campaign &campaign, const string &label) {
    cout << "  " << label << ": " << campaign.name() << " · " << campaign.kind() << "\n";
    for (const Channel &channel : campaign.channels()) {
        cout << "      " << channel.name << " ৳" << channel.budget << "\n";
    }
    cout << "      total ৳" << campaign.totalBudget() << "\n";
}

int main() {
    cout << "Prototype\n\n";

    CampaignRegistry registry;
    registry.add("eid-flash",
                 make_unique<FlashSaleCampaign>("Eid Flash Sale",
                                                vector<Channel>{{"push", 2000}, {"sms", 1500}},
                                                6));
    registry.add("signup",
                 make_unique<WelcomeCampaign>("Welcome Series",
                                              vector<Channel>{{"email", 300}, {"push", 100}},
                                              "new users"));

    cout << "Templates registered: " << registry.size() << "\n\n";

    // Two copies from the same template. Each owns its channels.
    unique_ptr<Campaign> dhaka = registry.create("eid-flash");
    unique_ptr<Campaign> chattogram = registry.create("eid-flash");

    dhaka->rename("Eid Flash Sale — Dhaka");
    dhaka->setBudget(0, 9000);
    chattogram->rename("Eid Flash Sale — Chattogram");
    chattogram->setBudget(1, 700);

    printCampaign(*dhaka, "copy A");
    cout << "\n";
    printCampaign(*chattogram, "copy B");
    cout << "\n";

    // The template is untouched: proving the clone is a deep copy.
    printCampaign(*registry.create("eid-flash"), "fresh copy from the template");

    cout << "\nA copy that rebuilds the object by hand would need to know it is a\n"
            "FlashSaleCampaign and repeat its 6 hours. clone() does not.\n";

    try {
        registry.create("black-friday");
    } catch (const out_of_range &error) {
        cout << "\nrefused: " << error.what() << "\n";
    }
    try {
        registry.add("signup", make_unique<WelcomeCampaign>("Dup", vector<Channel>{{"sms", 1}},
                                                            "dup"));
    } catch (const invalid_argument &error) {
        cout << "refused: " << error.what() << "\n";
    }

    return 0;
}
