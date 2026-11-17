#ifndef DATING_PUSH_CHANNEL_HPP
#define DATING_PUSH_CHANNEL_HPP

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "../notifications/NotificationChannel.hpp"
#include "../utils/ConsoleStyle.hpp"

class PushChannel : public NotificationChannel {
public:
    explicit PushChannel(std::string deviceId) : deviceId_(std::move(deviceId)) {
        if (deviceId_.empty()) {
            throw std::invalid_argument("PushChannel: device id must not be empty");
        }
    }

    std::string name() const override {
        return "push";
    }

    std::string destination() const override {
        return deviceId_;
    }

    void deliver(const std::string &userId, const std::string &message) const override {
        std::cout << "      " << Console::MAGENTA << Console::padRight("[push]", 9)
                  << Console::RESET << Console::padRight(deviceId_, 24) << Console::DIM << userId
                  << Console::RESET << "  → " << message << "\n";
    }

private:
    std::string deviceId_;
};

#endif // DATING_PUSH_CHANNEL_HPP
