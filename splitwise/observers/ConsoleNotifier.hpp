#ifndef SPLITWISE_CONSOLE_NOTIFIER_HPP
#define SPLITWISE_CONSOLE_NOTIFIER_HPP

#include <iostream>
#include <string>

#include "../app/UserDirectory.hpp"
#include "../money/Money.hpp"
#include "../utils/ConsoleStyle.hpp"
#include "GroupEvent.hpp"
#include "GroupListener.hpp"

// The listener the app actually uses: one line per recipient, on stdout.
//
// It borrows the directory read-only, to turn the ids in the event into names.
// It owns nothing and is never owned by a group - the facade holds it and
// subscribes it to every group it creates, so the whole app has one notifier
// instead of one per group.
class ConsoleNotifier : public GroupListener {
public:
    explicit ConsoleNotifier(const UserDirectory &directory) : directory_(directory) {}

    void onGroupEvent(const GroupEvent &event) override {
        const std::string line = summary(event);
        for (const UserId &recipient : event.recipients) {
            std::cout << "    " << Console::DIM << "→ "
                      << Console::padRight(directory_.get(recipient).name(), 8)
                      << Console::RESET << line << "\n";
        }
    }

private:
    std::string nameOf(const UserId &userId) const {
        return userId.empty() ? std::string("nobody") : directory_.get(userId).name();
    }

    std::string summary(const GroupEvent &event) const {
        switch (event.kind) {
            case GroupEvent::Kind::ExpenseAdded:
                return "\"" + event.description + "\" " + Money::format(event.amount) + " in " +
                       event.groupName + ", paid by " + nameOf(event.actor);
            case GroupEvent::Kind::Settlement:
                return nameOf(event.actor) + " settled " + Money::format(event.amount) + " with " +
                       nameOf(event.counterparty) + " in " + event.groupName;
            case GroupEvent::Kind::DebtsSimplified:
                return event.groupName + ": " + event.description;
        }
        return event.groupName;
    }

    const UserDirectory &directory_;
};

#endif // SPLITWISE_CONSOLE_NOTIFIER_HPP
