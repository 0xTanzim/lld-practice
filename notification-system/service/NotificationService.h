#ifndef NOTIFICATION_SERVICE_H
#define NOTIFICATION_SERVICE_H

#include "../notifications/INotification.h"
#include "../observers/NotificationObservable.h"
using namespace std;

class NotificationService {
private:
    NotificationObservable *observable;
    static NotificationService *instance;

    NotificationService() : observable(new NotificationObservable()) {}

public:
    static NotificationService *getInstance() {
        if (instance == nullptr) {
            instance = new NotificationService();
        }
        return instance;
    }

    NotificationObservable *getObservable() const {
        return observable;
    }

    void sendNotification(INotification *notification) {
        observable->setNotification(notification);
    }

    ~NotificationService() {
        delete observable;
    }
};

NotificationService *NotificationService::instance = nullptr;

#endif // NOTIFICATION_SERVICE_H
