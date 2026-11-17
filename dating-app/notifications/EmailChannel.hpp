#ifndef DATING_EMAIL_CHANNEL_HPP
#define DATING_EMAIL_CHANNEL_HPP

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "../notifications/NotificationChannel.hpp"
#include "../utils/ConsoleStyle.hpp"

class EmailChannel : public NotificationChannel {
public:
    explicit EmailChannel(std::string address) : address_(std::move(address)) {
        if (address_.empty() || address_.find('@') == std::string::npos) {
            throw std::invalid_argument("EmailChannel: address must look like name@host");
        }
    }

    std::string name() const override {
        return "email";
    }

    std::string destination() const override {
        return address_;
    }

    void deliver(const std::string &userId, const std::string &message) const override {
        std::cout << "      " << Console::BLUE << Console::padRight("[email]", 9)
                  << Console::RESET << Console::padRight(address_, 24) << Console::DIM << userId
                  << Console::RESET << "  → " << message << "\n";
    }

private:
    std::string address_;
};

#endif // DATING_EMAIL_CHANNEL_HPP
