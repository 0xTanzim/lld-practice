#ifndef NOTIFICATION_OBSERVABLE_H
#define NOTIFICATION_OBSERVABLE_H

#include <algorithm>
#include <stdexcept>
#include <vector>
#include "IObservable.h"
#include "../notifications/INotification.h"
using namespace std;

class NotificationObservable : public IObservable {
private:
    vector<IObserver *> observers;
    INotification *currentNotification = nullptr;

public:
    ~NotificationObservable() override {
        delete currentNotification;
    }

    void addObserver(IObserver *obs) override {
        observers.push_back(obs);
    }

    void removeObserver(IObserver *obs) override {
        observers.erase(remove(observers.begin(), observers.end(), obs), observers.end());
    }

    void notifyObservers() override {
        for (auto *obs : observers) {
            obs->update();
        }
    }

    void setNotification(INotification *notification) {
        delete currentNotification;
        currentNotification = notification;
        notifyObservers();
    }

    INotification *getNotification() const {
        return currentNotification;
    }

    string getNotificationContent() const {
        if (!currentNotification) {
            throw logic_error("No notification set yet.");
        }
        return currentNotification->getContent();
    }
};

#endif // NOTIFICATION_OBSERVABLE_H
