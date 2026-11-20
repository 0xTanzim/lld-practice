#ifndef SPLITWISE_USER_HPP
#define SPLITWISE_USER_HPP

#include <string>
#include <utility>

#include "../errors/AppError.hpp"
#include "Ids.hpp"

// A person who can pay and owe. No balances live here.
//
// The original stored each user's personal balance map on the user, so the same
// debt existed twice - once as `balances[A][B] = +200` and once as
// `balances[B][A] = -200` - and both copies had to be updated or they disagreed.
// Balances now belong to a Ledger, which keeps one entry per pair.
class User {
public:
    User(UserId id, std::string name, std::string email)
        : id_(std::move(id)), name_(std::move(name)), email_(std::move(email)) {
        if (id_.empty()) {
            throw AppError("user: id must not be empty");
        }
        if (name_.empty()) {
            throw AppError("user " + id_ + ": name must not be empty");
        }
        if (email_.find('@') == std::string::npos || email_.find(' ') != std::string::npos) {
            throw AppError("user " + id_ + ": email must look like name@host");
        }
    }

    const UserId &id() const {
        return id_;
    }

    const std::string &name() const {
        return name_;
    }

    const std::string &email() const {
        return email_;
    }

private:
    UserId id_;
    std::string name_;
    std::string email_;
};

#endif // SPLITWISE_USER_HPP
