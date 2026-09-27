#ifndef I_NOTIFICATION_H
#define I_NOTIFICATION_H

#include <string>
using namespace std;

class INotification {
public:
    virtual string getContent() const = 0;
    virtual ~INotification() = default;
};

#endif // I_NOTIFICATION_H
