#ifndef DATING_APP_HPP
#define DATING_APP_HPP

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../matching/Matcher.hpp"
#include "../matching/MatcherFactory.hpp"
#include "../models/ChatRoom.hpp"
#include "../models/Preference.hpp"
#include "../models/Profile.hpp"
#include "../models/Swipe.hpp"
#include "../models/User.hpp"
#include "../notifications/NotificationService.hpp"

// FACADE. One object the client talks to, hiding the four collaborators behind
// it: the user directory, the matcher, the chat rooms and the notification hub.
//
// The client never touches User, Matcher, ChatRoom or NotificationService
// directly for a feature flow - it calls one method and the facade decides what
// has to happen. Match detection, room creation and notifying both people are
// one call, not a sequence every caller has to remember.
class DatingApp {
public:
    struct Candidate {
        const User *user;
        double score;
    };

    explicit DatingApp(MatcherType type = MatcherType::LOCATION_BASED)
        : matcher_(MatcherFactory::create(type)) {}

    User &addUser(std::string id, Profile profile, Preference preference) {
        if (find(id) != nullptr) {
            throw AppError("user " + id + " already exists");
        }
        users_.push_back(
            std::make_unique<User>(std::move(id), std::move(profile), std::move(preference)));
        return *users_.back();
    }

    User *find(const std::string &id) {
        for (const std::unique_ptr<User> &user : users_) {
            if (user->id() == id) {
                return user.get();
            }
        }
        return nullptr;
    }

    const User *find(const std::string &id) const {
        for (const std::unique_ptr<User> &user : users_) {
            if (user->id() == id) {
                return user.get();
            }
        }
        return nullptr;
    }

    User &user(const std::string &id) {
        User *found = find(id);
        if (found == nullptr) {
            throw AppError("unknown user " + id);
        }
        return *found;
    }

    const User &user(const std::string &id) const {
        const User *found = find(id);
        if (found == nullptr) {
            throw AppError("unknown user " + id);
        }
        return *found;
    }

    void setMatcher(MatcherType type) {
        matcher_ = MatcherFactory::create(type);
    }

    const Matcher &matcher() const {
        return matcher_;
    }

    // People this user could still swipe on: inside the radius, not already
    // swiped, and passing the rules. Sorted by score, then id so the order is
    // stable when two candidates tie.
    std::vector<Candidate> candidates(const std::string &userId, double maxKm) const {
        if (!(maxKm > 0.0)) {
            throw std::invalid_argument("candidates: radius must be positive");
        }
        const User &viewer = user(userId);

        std::vector<Candidate> ranked;
        for (const std::unique_ptr<User> &candidate : users_) {
            if (candidate->id() == viewer.id()) {
                continue;
            }
            if (viewer.hasSwiped(candidate->id())) {
                continue;
            }
            const double distanceKm =
                viewer.profile().location().distanceKm(candidate->profile().location());
            if (distanceKm > maxKm) {
                continue;
            }
            const double score = matcher_.score(viewer, *candidate);
            if (score <= 0.0) {
                continue;
            }
            ranked.push_back(Candidate{candidate.get(), score});
        }

        std::sort(ranked.begin(), ranked.end(),
                  [](const Candidate &left, const Candidate &right) {
                      if (left.score != right.score) {
                          return left.score > right.score;
                      }
                      return left.user->id() < right.user->id();
                  });
        return ranked;
    }

    // Records the swipe and, when both sides liked each other, opens the chat
    // room and tells both people. Returns true only for a new match.
    bool swipe(const std::string &fromId, const std::string &toId, SwipeAction action) {
        User &viewer = user(fromId);
        User &target = user(toId);

        viewer.swipe(toId, action);  // throws on self-swipe and on a repeat swipe

        if (action != SwipeAction::RIGHT || !target.hasLiked(fromId)) {
            return false;
        }
        if (findRoom(fromId, toId) != nullptr) {
            throw std::logic_error("swipe: a chat room already exists for " + fromId + " and "
                                   + toId);
        }

        chatRooms_.emplace_back(roomIdFor(fromId, toId), fromId, toId);

        // Both people, every channel each of them has. The number of channels is
        // whatever they registered - the match code does not care.
        notifications_.notify(fromId, "It's a match with " + target.profile().name() + "!");
        notifications_.notify(toId, "It's a match with " + viewer.profile().name() + "!");
        return true;
    }

    const ChatRoom *findRoom(const std::string &firstId, const std::string &secondId) const {
        for (const ChatRoom &room : chatRooms_) {
            if (room.isBetween(firstId, secondId)) {
                return &room;
            }
        }
        return nullptr;
    }

    const ChatRoom &chatRoom(const std::string &firstId, const std::string &secondId) const {
        const ChatRoom *room = findRoom(firstId, secondId);
        if (room == nullptr) {
            throw AppError("no chat room between " + firstId + " and " + secondId);
        }
        return *room;
    }

    // Sending a message requires that the two people already matched. The
    // original flipped that around: it looked up a room, and when there was none
    // it printed a line and returned as if nothing had happened.
    void sendMessage(const std::string &fromId, const std::string &toId, const std::string &text) {
        const User &sender = user(fromId);
        user(toId);  // existence check; the room lookup would not catch a bad receiver

        ChatRoom &room = writableRoom(fromId, toId);
        room.addMessage(fromId, text);
        notifications_.notify(toId, "New message from " + sender.profile().name());
    }

    std::size_t userCount() const {
        return users_.size();
    }

    std::size_t chatRoomCount() const {
        return chatRooms_.size();
    }

    const std::vector<std::unique_ptr<User>> &users() const {
        return users_;
    }

    const std::vector<ChatRoom> &chatRooms() const {
        return chatRooms_;
    }

    NotificationService &notifications() {
        return notifications_;
    }

    const NotificationService &notifications() const {
        return notifications_;
    }

private:
    static std::string roomIdFor(const std::string &firstId, const std::string &secondId) {
        return firstId < secondId ? firstId + "--" + secondId : secondId + "--" + firstId;
    }

    ChatRoom &writableRoom(const std::string &firstId, const std::string &secondId) {
        for (ChatRoom &room : chatRooms_) {
            if (room.isBetween(firstId, secondId)) {
                return room;
            }
        }
        throw AppError("no chat room between " + firstId + " and " + secondId);
    }

    // Declaration order matters here: matcher_ is initialised in the constructor,
    // and notifications_ must outlive nothing in particular, but keeping the
    // collaborators together makes the facade's ownership obvious.
    std::vector<std::unique_ptr<User>> users_;
    std::vector<ChatRoom> chatRooms_;
    Matcher matcher_;
    NotificationService notifications_;
};

#endif // DATING_APP_HPP
