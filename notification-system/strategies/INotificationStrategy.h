#ifndef I_NOTIFICATION_STRATEGY_H
#define I_NOTIFICATION_STRATEGY_H

#include <string>
using namespace std;

class INotificationStrategy {
public:
    virtual void sendNotification(const string &content) = 0;
    virtual ~INotificationStrategy() = default;
};

#endif // I_NOTIFICATION_STRATEGY_H
