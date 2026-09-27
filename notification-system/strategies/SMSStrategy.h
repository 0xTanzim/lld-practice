#ifndef SMS_STRATEGY_H
#define SMS_STRATEGY_H

#include <iostream>
#include "INotificationStrategy.h"

class SMSStrategy : public INotificationStrategy {
private:
    string mobileNumber;

public:
    explicit SMSStrategy(const string &mobileNumber) : mobileNumber(mobileNumber) {}

    void sendNotification(const string &content) override {
        cout << "Sending SMS Notification to: " << mobileNumber << "\n" << content << endl;
    }
};

#endif // SMS_STRATEGY_H
