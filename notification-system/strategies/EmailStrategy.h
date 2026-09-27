#ifndef EMAIL_STRATEGY_H
#define EMAIL_STRATEGY_H

#include <iostream>
#include "INotificationStrategy.h"

class EmailStrategy : public INotificationStrategy {
private:
    string emailId;

public:
    explicit EmailStrategy(const string &emailId) : emailId(emailId) {}

    void sendNotification(const string &content) override {
        cout << "Sending email Notification to: " << emailId << "\n" << content << endl;
    }
};

#endif // EMAIL_STRATEGY_H
