#ifndef SIGNATURE_DECORATOR_H
#define SIGNATURE_DECORATOR_H

#include "INotificationDecorator.h"

class SignatureDecorator : public INotificationDecorator {
private:
    string signature;

public:
    SignatureDecorator(INotification *n, const string &sig)
        : INotificationDecorator(n), signature(sig) {}

    string getContent() const override {
        return notification->getContent() + "\n-- " + signature + "\n";
    }
};

#endif // SIGNATURE_DECORATOR_H
