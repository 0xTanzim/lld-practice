#ifndef DATING_NOTIFICATION_CHANNEL_HPP
#define DATING_NOTIFICATION_CHANNEL_HPP

#include <string>

// One way of reaching one person: an email address, a phone number, a device.
//
// This is the Observer half of the design. The subject is NotificationService,
// the observers are channels. The original had exactly the same shape but stored
// ONE observer per user in a map, so registering a second channel overwrote the
// first - that is the "we lose a couple of notifications" bug.
class NotificationChannel {
public:
    virtual ~NotificationChannel() = default;

    // Short stable key: "email", "sms", "push". Used to keep one channel of each
    // kind per user, and to remove one without disturbing the others.
    virtual std::string name() const = 0;

    // Where this channel sends: an address, a number, a device id.
    virtual std::string destination() const = 0;

    virtual void deliver(const std::string &userId, const std::string &message) const = 0;
};

#endif // DATING_NOTIFICATION_CHANNEL_HPP
