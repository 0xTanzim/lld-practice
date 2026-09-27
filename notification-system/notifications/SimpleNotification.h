#ifndef SIMPLE_NOTIFICATION_H
#define SIMPLE_NOTIFICATION_H

#include "INotification.h"

class SimpleNotification : public INotification {
private:
    string text;

public:
    explicit SimpleNotification(const string &msg) : text(msg) {}

    string getContent() const override {
        return text;
    }
};

#endif // SIMPLE_NOTIFICATION_H
