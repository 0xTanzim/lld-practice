#ifndef SPLITWISE_GROUP_LISTENER_HPP
#define SPLITWISE_GROUP_LISTENER_HPP

#include "GroupEvent.hpp"

// OBSERVER. A group publishes, listeners react; the group never knows what they
// do or how many there are.
//
// The original made every group member an Observer and stored the balance on the
// member. Reacting to an expense and holding that member's money were the same
// object, so a member could not be watched, and being watched could not be
// turned off without changing who owed what.
class GroupListener {
public:
    virtual ~GroupListener() = default;

    virtual void onGroupEvent(const GroupEvent &event) = 0;
};

#endif // SPLITWISE_GROUP_LISTENER_HPP
