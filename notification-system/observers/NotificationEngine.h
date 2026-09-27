#ifndef NOTIFICATION_ENGINE_H
#define NOTIFICATION_ENGINE_H

#include <vector>
#include "IObserver.h"
#include "../service/NotificationService.h"
#include "../strategies/INotificationStrategy.h"
using namespace std;

class NotificationEngine : public IObserver {
private:
    NotificationObservable *notificationObservable;
    vector<INotificationStrategy *> notificationStrategies;

public:
    NotificationEngine() : NotificationEngine(NotificationService::getInstance()->getObservable()) {}

    explicit NotificationEngine(NotificationObservable *observable)
        : notificationObservable(observable) {
        notificationObservable->addObserver(this);
    }

    ~NotificationEngine() override {
        notificationObservable->removeObserver(this);
        for (auto *strategy : notificationStrategies) {
            delete strategy;
        }
    }

    void addNotificationStrategy(INotificationStrategy *ns) {
        notificationStrategies.push_back(ns);
    }

    void update() override {
        string content = notificationObservable->getNotificationContent();
        for (auto *strategy : notificationStrategies) {
            strategy->sendNotification(content);
        }
    }
};

#endif // NOTIFICATION_ENGINE_H
