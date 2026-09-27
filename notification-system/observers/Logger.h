#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include "IObserver.h"
#include "../service/NotificationService.h"
using namespace std;

class Logger : public IObserver {
private:
    NotificationObservable *notificationObservable;

public:
    Logger() : Logger(NotificationService::getInstance()->getObservable()) {}

    explicit Logger(NotificationObservable *observable)
        : notificationObservable(observable) {
        notificationObservable->addObserver(this);
    }

    ~Logger() override {
        notificationObservable->removeObserver(this);
    }

    void update() override {
        cout << "Logging New Notification :\n"
             << notificationObservable->getNotificationContent() << endl;
    }
};

#endif // LOGGER_H
