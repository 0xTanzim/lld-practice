#ifndef I_OBSERVABLE_H
#define I_OBSERVABLE_H

#include "IObserver.h"

class IObservable {
public:
    virtual void addObserver(IObserver *observer) = 0;
    virtual void removeObserver(IObserver *observer) = 0;
    virtual void notifyObservers() = 0;
    virtual ~IObservable() = default;
};

#endif // I_OBSERVABLE_H
