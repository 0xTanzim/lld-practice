#ifndef DATING_SMS_CHANNEL_HPP
#define DATING_SMS_CHANNEL_HPP

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "../notifications/NotificationChannel.hpp"
#include "../utils/ConsoleStyle.hpp"

class SmsChannel : public NotificationChannel {
public:
    explicit SmsChannel(std::string number) : number_(std::move(number)) {
        if (number_.empty()) {
            throw std::invalid_argument("SmsChannel: number must not be empty");
        }
        for (char character : number_) {
            if (character != '+' && character != '-' && character != ' '
                && (character < '0' || character > '9')) {
                throw std::invalid_argument("SmsChannel: number may only contain digits, "
                                            "spaces, '+' and '-'");
            }
        }
    }

    std::string name() const override {
        return "sms";
    }

    std::string destination() const override {
        return number_;
    }

    void deliver(const std::string &userId, const std::string &message) const override {
        std::cout << "      " << Console::YELLOW << Console::padRight("[sms]", 9)
                  << Console::RESET << Console::padRight(number_, 24) << Console::DIM << userId
                  << Console::RESET << "  → " << message << "\n";
    }

private:
    std::string number_;
};

#endif // DATING_SMS_CHANNEL_HPP
