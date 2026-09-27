#ifndef TIMESTAMP_DECORATOR_H
#define TIMESTAMP_DECORATOR_H

#include <ctime>
#include "INotificationDecorator.h"

class TimestampDecorator : public INotificationDecorator {
public:
    explicit TimestampDecorator(INotification *n) : INotificationDecorator(n) {}

    string getContent() const override {
        return "[" + currentTime() + "] " + notification->getContent();
    }

private:
    static string currentTime() {
        time_t now = time(nullptr);
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return buffer;
    }
};

#endif // TIMESTAMP_DECORATOR_H
