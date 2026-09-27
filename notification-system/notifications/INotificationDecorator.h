#ifndef I_NOTIFICATION_DECORATOR_H
#define I_NOTIFICATION_DECORATOR_H

#include <stdexcept>
#include "INotification.h"

class INotificationDecorator : public INotification {
protected:
    INotification *notification;

public:
    explicit INotificationDecorator(INotification *n) : notification(n) {
        if (!notification) {
            throw invalid_argument("Decorator requires a wrapped notification.");
        }
    }

    ~INotificationDecorator() override {
        delete notification;
    }
};

#endif // I_NOTIFICATION_DECORATOR_H
