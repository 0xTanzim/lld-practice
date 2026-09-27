#ifndef POP_UP_STRATEGY_H
#define POP_UP_STRATEGY_H

#include <iostream>
#include "INotificationStrategy.h"

class PopUpStrategy : public INotificationStrategy {
public:
    void sendNotification(const string &content) override {
        cout << "Sending Popup Notification: \n" << content << endl;
    }
};

#endif // POP_UP_STRATEGY_H
