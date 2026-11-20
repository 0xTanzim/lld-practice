#ifndef SPLITWISE_APP_ERROR_HPP
#define SPLITWISE_APP_ERROR_HPP

#include <stdexcept>
#include <string>

// Every way an operation can be refused. One type: callers print the message and
// never branch on the class.
//
// The original was not consistent about this. `addExpense` threw, `settlePayment`
// printed "user is not a part of this group" and returned false, `removeMember`
// printed and returned false, and `addUserToGroup` silently did nothing when the
// user or group was missing. A caller had no way to know which behaviour to
// expect, and the quiet paths looked like success in the output.
class AppError : public std::runtime_error {
public:
    explicit AppError(const std::string &message) : std::runtime_error(message) {}
};

#endif // SPLITWISE_APP_ERROR_HPP
