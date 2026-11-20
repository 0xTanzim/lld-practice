#ifndef SPLITWISE_GROUP_EVENT_HPP
#define SPLITWISE_GROUP_EVENT_HPP

#include <string>
#include <utility>
#include <vector>

#include "../money/Money.hpp"
#include "../models/Ids.hpp"

// One thing that happened in a group, described without any console formatting.
//
// The event carries ids, never User pointers and never printed strings: whoever
// receives it decides how to render it, and a listener that only counts events
// never needs to resolve a name.
struct GroupEvent {
    enum class Kind { ExpenseAdded, Settlement, DebtsSimplified };

    Kind kind = Kind::ExpenseAdded;
    GroupId groupId;
    std::string groupName;

    // Who did it. Empty for a simplification, which nobody performed.
    UserId actor;

    // The other side: who was paid. Empty unless this is a settlement.
    UserId counterparty;

    // The human sentence for the kind: the expense description, or the
    // before/after transaction count of a simplification.
    std::string description;

    // Only meaningful for ExpenseAdded and Settlement.
    Money::Amount amount = 0;

    // Everyone the group must tell. Already resolved by the group, so a
    // listener does not need to reach back into it.
    std::vector<UserId> recipients;
};

inline std::string name(GroupEvent::Kind kind) {
    switch (kind) {
        case GroupEvent::Kind::ExpenseAdded:
            return "expense";
        case GroupEvent::Kind::Settlement:
            return "settlement";
        case GroupEvent::Kind::DebtsSimplified:
            return "simplification";
    }
    return "unknown";
}

#endif // SPLITWISE_GROUP_EVENT_HPP
