#ifndef DATING_MESSAGE_HPP
#define DATING_MESSAGE_HPP

#include <ctime>
#include <stdexcept>
#include <string>
#include <utility>

// One chat message. The timestamp comes from the system clock by default; a
// caller that supplies one gets a deterministic value (the demo does this so its
// output does not change between runs).
class Message {
public:
    Message(std::string senderId, std::string content, std::time_t sentAt)
        : senderId_(std::move(senderId)), content_(std::move(content)), sentAt_(sentAt) {
        if (senderId_.empty()) {
            throw std::invalid_argument("Message: sender must not be empty");
        }
        if (content_.empty()) {
            throw std::invalid_argument("Message: content must not be empty");
        }
    }

    static Message now(std::string senderId, std::string content) {
        return Message(std::move(senderId), std::move(content), std::time(nullptr));
    }

    const std::string &senderId() const {
        return senderId_;
    }

    const std::string &content() const {
        return content_;
    }

    std::time_t sentAt() const {
        return sentAt_;
    }

    std::string formattedTime() const {
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &sentAt_);
#else
        localtime_r(&sentAt_, &local);
#endif
        char buffer[16];
        if (std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &local) == 0) {
            throw std::runtime_error("Message: timestamp could not be formatted");
        }
        return buffer;
    }

private:
    std::string senderId_;
    std::string content_;
    std::time_t sentAt_;
};

#endif // DATING_MESSAGE_HPP
