#include <cstddef>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "app/DatingApp.hpp"
#include "errors/AppError.hpp"
#include "matching/Matcher.hpp"
#include "matching/MatcherFactory.hpp"
#include "models/ChatRoom.hpp"
#include "models/Gender.hpp"
#include "models/Interest.hpp"
#include "models/Location.hpp"
#include "models/Message.hpp"
#include "models/Preference.hpp"
#include "models/Profile.hpp"
#include "models/Swipe.hpp"
#include "models/User.hpp"
#include "notifications/EmailChannel.hpp"
#include "notifications/NotificationService.hpp"
#include "notifications/PushChannel.hpp"
#include "notifications/SmsChannel.hpp"
#include "utils/ConsoleStyle.hpp"

using namespace std;

namespace {

constexpr double NEARBY_RADIUS_KM = 5.0;
constexpr double WIDER_RADIUS_KM = 10.0;

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

string formatKm(double kilometres) {
    ostringstream out;
    out << fixed << setprecision(1) << kilometres << " km";
    return out.str();
}

string formatScore(double score) {
    ostringstream out;
    out << fixed << setprecision(2) << score;
    return out.str();
}

string scoreBar(double score) {
    const size_t width = 20;
    const size_t filled = static_cast<size_t>(score * static_cast<double>(width) + 0.5);
    return Console::repeat("█", filled) + Console::DIM + Console::repeat("·", width - filled)
           + Console::RESET;
}

Profile buildProfile(const string &name, int age, Gender gender, const string &bio, double latitude,
                     double longitude, initializer_list<Interest> interests) {
    Profile profile(name, age, gender, bio, Location(latitude, longitude));
    for (const Interest &interest : interests) {
        profile.addInterest(interest);
    }
    return profile;
}

void printUser(const User &user) {
    const Profile &profile = user.profile();
    const Preference &preference = user.preference();

    cout << "    " << Console::BOLD << profile.name() << Console::RESET << Console::DIM << "  ("
         << user.id() << ")" << Console::RESET << "\n";
    cout << "      " << profile.age() << " · " << toString(profile.gender()) << " · "
         << profile.location().format() << "\n";
    cout << "      " << Console::DIM << profile.bio() << Console::RESET << "\n";

    cout << "      interests: ";
    bool first = true;
    for (const Interest &interest : profile.interests()) {
        if (!first) {
            cout << " · ";
        }
        cout << interest.name() << "/" << interest.category();
        first = false;
    }
    if (first) {
        cout << "—";
    }
    cout << "\n";

    cout << "      wants: ";
    if (preference.interestedIn().empty()) {
        cout << "anyone";
    } else {
        bool firstGender = true;
        for (Gender gender : preference.interestedIn()) {
            if (!firstGender) {
                cout << ", ";
            }
            cout << toString(gender);
            firstGender = false;
        }
    }
    cout << " · age " << preference.minAge() << "-" << preference.maxAge() << " · within "
         << formatKm(preference.maxDistanceKm()) << "\n";
}

void printCandidates(const vector<DatingApp::Candidate> &candidates) {
    if (candidates.empty()) {
        Console::info("no candidates");
        return;
    }
    for (const DatingApp::Candidate &candidate : candidates) {
        cout << "    " << Console::padRight(candidate.user->profile().name(), 10)
             << Console::DIM << Console::padRight(candidate.user->id(), 12) << Console::RESET
             << scoreBar(candidate.score) << "  " << Console::BOLD << formatScore(candidate.score)
             << Console::RESET << "\n";
    }
}

void printChannels(const NotificationService &hub, const string &userId) {
    const vector<string> names = hub.channelNames(userId);
    cout << "    " << Console::padRight(userId, 12) << hub.channelCount(userId) << " channel(s): ";
    if (names.empty()) {
        cout << Console::DIM << "—" << Console::RESET;
    }
    for (size_t index = 0; index < names.size(); ++index) {
        if (index > 0) {
            cout << ", ";
        }
        cout << names[index];
    }
    cout << "\n";
}

void printRoom(const DatingApp &app, const string &firstId, const string &secondId) {
    const ChatRoom &room = app.chatRoom(firstId, secondId);
    cout << "    " << Console::BOLD << "room " << room.id() << Console::RESET << Console::DIM
         << "  (" << room.messageCount() << " messages)" << Console::RESET << "\n";
    for (const Message &message : room.messages()) {
        const User &sender = app.user(message.senderId());
        cout << "      " << Console::DIM << message.formattedTime() << Console::RESET << "  "
             << Console::padRight(sender.profile().name(), 10) << "  " << message.content()
             << "\n";
    }
}

}

int main() {
    Console::banner("DATING APP — MATCHING · NOTIFICATIONS · CHAT");

    Checks checks;
    DatingApp app(MatcherType::LOCATION_BASED);

    // ---------------------------------------------------------------- 1
    Console::section("1. Profiles");

    Profile rohanProfile =
        buildProfile("Rohan", 28, Gender::MALE, "Backend developer, night owl.", 1.0100, 1.0200,
                     {Interest("Coding", "Programming"), Interest("Travel", "Lifestyle"),
                      Interest("Music", "Entertainment")});
    rohanProfile.addPhoto("rohan_1.jpg");
    User &rohan = app.addUser("u-rohan", std::move(rohanProfile),
                              Preference({Gender::FEMALE}, 24, 32, 10.0));

    Profile nehaProfile =
        buildProfile("Neha", 27, Gender::FEMALE, "Art teacher who paints on weekends.", 1.0300,
                     1.0400, {Interest("Painting", "Art"), Interest("Travel", "Lifestyle"),
                              Interest("Music", "Entertainment")});
    nehaProfile.addPhoto("neha_1.jpg");
    User &neha = app.addUser("u-neha", std::move(nehaProfile),
                             Preference({Gender::MALE}, 26, 34, 15.0));

    User &tanvir = app.addUser(
        "u-tanvir",
        buildProfile("Tanvir", 30, Gender::MALE, "Cricket, coffee, compilers.", 1.0120, 1.0220,
                     {Interest("Coding", "Programming"), Interest("Music", "Entertainment"),
                      Interest("Cricket", "Sports")}),
        Preference({Gender::FEMALE}, 25, 30, 5.0));

    User &ayesha = app.addUser(
        "u-ayesha",
        buildProfile("Ayesha", 26, Gender::FEMALE, "Travel blogger chasing cheap flights.", 1.0500,
                     1.0700, {Interest("Travel", "Lifestyle"), Interest("Painting", "Art")}),
        Preference({Gender::MALE}, 27, 33, 20.0));

    for (const unique_ptr<User> &user : app.users()) {
        printUser(*user);
        cout << "\n";
    }

    Console::kv("distances from Rohan",
                "Neha " + formatKm(rohan.profile().location().distanceKm(neha.profile().location()))
                    + " · Tanvir "
                    + formatKm(rohan.profile().location().distanceKm(tanvir.profile().location()))
                    + " · Ayesha "
                    + formatKm(rohan.profile().location().distanceKm(ayesha.profile().location())));
    checks.expect(app.userCount() == 4, "four users registered");
    checks.expect(rohan.profile().interests().size() == 3, "Rohan has three interests");

    // ---------------------------------------------------------------- 2
    Console::section("2. Same pair, three matchers");

    for (MatcherType type :
         {MatcherType::BASIC, MatcherType::INTERESTS_BASED, MatcherType::LOCATION_BASED}) {
        Matcher matcher = MatcherFactory::create(type);
        cout << "    " << Console::BOLD << Console::padRight(MatcherFactory::name(type), 10)
             << Console::RESET << Console::DIM << matcher.pipeline() << Console::RESET << "\n";
        cout << "      Rohan → Neha    " << scoreBar(matcher.score(rohan, neha)) << "  "
             << formatScore(matcher.score(rohan, neha)) << "\n";
        cout << "      Rohan → Ayesha  " << scoreBar(matcher.score(rohan, ayesha)) << "  "
             << formatScore(matcher.score(rohan, ayesha)) << "\n";
        cout << "      Rohan → Tanvir  " << scoreBar(matcher.score(rohan, tanvir)) << "  "
             << formatScore(matcher.score(rohan, tanvir)) << "\n";
    }

    const Matcher pipeline = MatcherFactory::create(MatcherType::LOCATION_BASED);
    checks.expect(pipeline.score(rohan, neha) > pipeline.score(rohan, ayesha),
                  "shared interests and distance rank Neha above Ayesha");
    checks.expect(pipeline.score(rohan, tanvir) == 0.0,
                  "Rohan only wants women, so Tanvir scores 0 under every matcher");
    checks.expect(pipeline.score(rohan, rohan) == 0.0, "a user never matches themselves");
    checks.expect(MatcherFactory::create(MatcherType::BASIC).score(rohan, neha) == 0.5,
                  "basic matcher gives a flat 0.5 to a compatible pair");

    // ---------------------------------------------------------------- 3
    Console::section("3. One user, many channels — the bug this project fixes");

    NotificationService &hub = app.notifications();

    cout << "    " << Console::DIM
         << "original: observers[userId] = observer;   // second channel overwrote the first"
         << Console::RESET << "\n\n";

    hub.subscribe("u-rohan", make_unique<EmailChannel>("rohan@mail.dev"));
    printChannels(hub, "u-rohan");
    size_t delivered = hub.notify("u-rohan", "Welcome to the app");
    checks.expect(delivered == 1, "one channel → one delivery");

    cout << "\n";
    hub.subscribe("u-rohan", make_unique<PushChannel>("pixel-7"));
    printChannels(hub, "u-rohan");
    delivered = hub.notify("u-rohan", "Your profile is 80% complete");
    checks.expect(delivered == 2, "adding push keeps email → two deliveries");
    checks.expect(hub.channelCount("u-rohan") == 2, "email was not replaced by push");

    cout << "\n";
    hub.subscribe("u-rohan", make_unique<SmsChannel>("+8801711000000"));
    printChannels(hub, "u-rohan");
    delivered = hub.notify("u-rohan", "5 people viewed your profile today");
    checks.expect(delivered == 3, "adding sms → three deliveries, nothing lost");

    cout << "\n";
    hub.unsubscribe("u-rohan", "push");
    printChannels(hub, "u-rohan");
    delivered = hub.notify("u-rohan", "Push is off, email and sms still work");
    checks.expect(delivered == 2, "removing one channel leaves the other two");
    checks.expect(hub.channelNames("u-rohan") == vector<string>({"email", "sms"}),
                  "the remaining channels are the ones we kept");

    hub.subscribe("u-neha", make_unique<SmsChannel>("+8801811111111"));
    hub.subscribe("u-tanvir", make_unique<PushChannel>("tanvir-tab"));
    cout << "\n";
    printChannels(hub, "u-neha");
    printChannels(hub, "u-tanvir");
    checks.expect(hub.subscriberCount() == 3, "three users have channels");
    checks.expect(hub.channelTotal() == 4, "four channels in total");

    // ---------------------------------------------------------------- 4
    Console::section("4. Candidates for Rohan inside 5 km");

    const vector<DatingApp::Candidate> nearby = app.candidates("u-rohan", NEARBY_RADIUS_KM);
    printCandidates(nearby);
    checks.expect(nearby.size() == 1, "only Neha is close enough and passes the rules");
    checks.expect(nearby.front().user->id() == "u-neha", "the one candidate is Neha");
    checks.expect(nearby.front().score > 0.7, "Neha clears the 0.70 line");

    // ---------------------------------------------------------------- 5
    Console::section("5. Widen the radius to 10 km: Ayesha enters");

    const vector<DatingApp::Candidate> wider = app.candidates("u-rohan", WIDER_RADIUS_KM);
    printCandidates(wider);
    checks.expect(wider.size() == 2, "Ayesha appears once the radius covers her");
    checks.expect(wider.front().user->id() == "u-neha", "the better score still sorts first");

    // ---------------------------------------------------------------- 6
    Console::section("6. Swipe right, twice");

    const bool oneSided = app.swipe("u-rohan", "u-neha", SwipeAction::RIGHT);
    checks.expect(!oneSided, "a one-sided like is not a match");
    checks.expect(app.chatRoomCount() == 0, "no chat room yet");

    const bool mutual = app.swipe("u-neha", "u-rohan", SwipeAction::RIGHT);
    checks.expect(mutual, "the second like creates the match");
    checks.expect(app.chatRoomCount() == 1, "exactly one chat room");
    checks.expect(app.chatRoom("u-neha", "u-rohan").messageCount() == 0, "the room starts empty");

    // The point of taking notification wiring out of User: activity on a User
    // cannot disturb its channel registration.
    checks.expect(hub.channelCount("u-rohan") == 2,
                  "swiping did not disturb Rohan's channel registration");
    checks.expect(hub.channelCount("u-neha") == 1, "Neha still has her one channel");

    // ---------------------------------------------------------------- 7
    Console::section("7. Chat inside the matched room");

    app.sendMessage("u-rohan", "u-neha", "Hi Neha, kaise ho?");
    app.sendMessage("u-neha", "u-rohan", "Badiya! Tum batao?");
    printRoom(app, "u-rohan", "u-neha");

    checks.expect(app.chatRoom("u-rohan", "u-neha").messageCount() == 2, "two messages recorded");
    checks.expect(app.chatRoom("u-neha", "u-rohan").messages().front().senderId() == "u-rohan",
                  "messages keep their order");
    checks.expect(rohan.swipeCount() == 1 && neha.swipeCount() == 1, "each user has one swipe");

    // ---------------------------------------------------------------- 8
    Console::section("8. Refusals");

    string reason;

    if (refuses([&] { app.swipe("u-rohan", "u-rohan", SwipeAction::RIGHT); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("yourself") != string::npos, "self-swipe refused");
    }

    if (refuses([&] { app.swipe("u-rohan", "u-neha", SwipeAction::LEFT); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("already swiped") != string::npos,
                      "a second swipe on the same person refused");
    }

    if (refuses([&] { app.sendMessage("u-tanvir", "u-neha", "hey"); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("no chat room") != string::npos,
                      "messaging without a match refused");
    }

    if (refuses([&] { app.user("u-ghost"); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("unknown user") != string::npos, "unknown user refused");
    }

    if (refuses(
            [&] {
                app.addUser("u-rohan",
                            buildProfile("Duplicate", 30, Gender::OTHER, "x", 1.0, 1.0, {}),
                            Preference({}, 20, 40, 5.0));
            },
            reason)) {
        Console::warn(reason);
        checks.expect(reason.find("already exists") != string::npos, "duplicate user id refused");
    }

    if (refuses([&] { Profile("Too Young", 17, Gender::OTHER, "x", Location(1.0, 1.0)); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("age must be between") != string::npos,
                      "a profile below 18 is refused");
    }

    if (refuses([&] { Location(100.0, 0.0); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("latitude") != string::npos, "an impossible latitude is refused");
    }

    hub.subscribe("u-neha", make_unique<PushChannel>("neha-tab"));
    if (refuses([&] { hub.subscribe("u-neha", make_unique<SmsChannel>("+8801999999999")); },
                reason)) {
        Console::warn(reason);
        checks.expect(reason.find("already has") != string::npos,
                      "a second channel of the same kind is refused");
    }

    if (refuses([&] { hub.unsubscribe("u-neha", "whatsapp"); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("no whatsapp channel") != string::npos,
                      "removing a channel that was never registered is refused");
    }

    if (refuses([&] { MatcherFactory::create(static_cast<MatcherType>(99)); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("unknown matcher type") != string::npos,
                      "an unknown matcher type is refused, not downgraded");
    }

    if (refuses([&] { app.candidates("u-rohan", 0.0); }, reason)) {
        Console::warn(reason);
        checks.expect(reason.find("radius must be positive") != string::npos,
                      "a non-positive radius is refused");
    }

    // ---------------------------------------------------------------- 9
    Console::section("9. Broadcast to every subscriber");
    const size_t broadcast = hub.notifyAll("Scheduled maintenance tonight at 02:00");
    Console::kv("deliveries", to_string(broadcast));
    checks.expect(broadcast == hub.channelTotal(), "the broadcast reaches every channel");

    // ---------------------------------------------------------------- 10
    Console::section("10. Final state");
    for (const unique_ptr<User> &user : app.users()) {
        cout << "    " << Console::padRight(user->profile().name(), 10) << Console::DIM
             << Console::padRight(user->id(), 12) << Console::RESET << "swipes "
             << user->swipeCount() << " · channels " << hub.channelCount(user->id()) << "\n";
    }
    Console::divider(70);
    Console::kv("users", to_string(app.userCount()));
    Console::kv("chat rooms", to_string(app.chatRoomCount()));
    Console::kv("subscribers", to_string(hub.subscriberCount()));
    Console::kv("channels", to_string(hub.channelTotal()));
    Console::kv("match pipeline", app.matcher().pipeline());

    Console::divider(70);
    if (checks.failed == 0) {
        Console::ok(to_string(checks.passed) + " checks passed");
        return 0;
    }
    Console::fail(to_string(checks.failed) + " of " + to_string(checks.passed + checks.failed)
                  + " checks failed");
    return 1;
}
