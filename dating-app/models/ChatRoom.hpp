#ifndef DATING_CHAT_ROOM_HPP
#define DATING_CHAT_ROOM_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "Message.hpp"

// The conversation created when two people match. Messages are stored by value:
// the original held `vector<Message*>` and deleted them in the destructor, which
// leaked on an exception and double-freed if the room was ever copied.
class ChatRoom {
public:
    ChatRoom(std::string id, std::string firstUserId, std::string secondUserId)
        : id_(std::move(id)), participants_{std::move(firstUserId), std::move(secondUserId)} {
        if (id_.empty()) {
            throw std::invalid_argument("ChatRoom: id must not be empty");
        }
        if (participants_[0].empty() || participants_[1].empty()) {
            throw std::invalid_argument("ChatRoom: participant id must not be empty");
        }
        if (participants_[0] == participants_[1]) {
            throw std::invalid_argument("ChatRoom: a room needs two different people");
        }
    }

    const std::string &id() const {
        return id_;
    }

    const std::vector<std::string> &participants() const {
        return participants_;
    }

    const std::vector<Message> &messages() const {
        return messages_;
    }

    bool hasParticipant(const std::string &userId) const {
        for (const std::string &participant : participants_) {
            if (participant == userId) {
                return true;
            }
        }
        return false;
    }

    // True only for the exact pair this room was created for, in either order.
    bool isBetween(const std::string &first, const std::string &second) const {
        return (participants_[0] == first && participants_[1] == second)
               || (participants_[0] == second && participants_[1] == first);
    }

    void addMessage(std::string senderId, std::string content) {
        addMessage(std::move(senderId), std::move(content), std::time(nullptr));
    }

    void addMessage(std::string senderId, std::string content, std::time_t sentAt) {
        if (!hasParticipant(senderId)) {
            throw AppError("sender " + senderId + " is not in room " + id_);
        }
        messages_.emplace_back(std::move(senderId), std::move(content), sentAt);
    }

    std::size_t messageCount() const {
        return messages_.size();
    }

private:
    std::string id_;
    std::vector<std::string> participants_;
    std::vector<Message> messages_;
};

#endif // DATING_CHAT_ROOM_HPP
