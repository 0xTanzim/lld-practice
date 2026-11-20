#ifndef SPLITWISE_USER_DIRECTORY_HPP
#define SPLITWISE_USER_DIRECTORY_HPP

#include <cctype>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "../errors/AppError.hpp"
#include "../models/Ids.hpp"
#include "../models/User.hpp"

// Everyone who can pay or owe, owned in one place.
//
// The original kept a `map<string, User*>` of raw pointers and handed them out,
// so `getUser` returned nullptr for a typo and every caller decided for itself
// whether to check. Here `get` throws and `find` is the explicit optional form,
// and the directory owns the users, so nobody has to free them.
//
// Ids are generated rather than accepted: `user1`, `user2`, ... A caller that
// supplies its own id can collide with one that already exists, which is exactly
// what a duplicate-email check is meant to prevent.
class UserDirectory {
public:
    const User &add(std::string name, std::string email) {
        const UserId id = "user" + std::to_string(nextId_);

        // Construct first: User validates its own fields, and a rejected user
        // must not consume an id or reserve an email address.
        User user(id, std::move(name), std::move(email));

        if (!emails_.insert(normalized(user.email())).second) {
            throw AppError("user " + user.email() + " is already registered as another user");
        }

        ++nextId_;
        return users_.emplace(id, std::move(user)).first->second;
    }

    const User *find(const UserId &userId) const {
        const auto found = users_.find(userId);
        return found == users_.end() ? nullptr : &found->second;
    }

    const User &get(const UserId &userId) const {
        const User *found = find(userId);
        if (found == nullptr) {
            throw AppError("unknown user " + userId);
        }
        return *found;
    }

    bool contains(const UserId &userId) const {
        return users_.find(userId) != users_.end();
    }

    const std::map<UserId, User> &users() const {
        return users_;
    }

    std::size_t count() const {
        return users_.size();
    }

private:
    static std::string normalized(const std::string &email) {
        std::string lowercased = email;
        for (char &letter : lowercased) {
            letter = static_cast<char>(
                std::tolower(static_cast<unsigned char>(letter)));
        }
        return lowercased;
    }

    std::size_t nextId_ = 1;
    std::map<UserId, User> users_;
    std::set<std::string> emails_;
};

#endif // SPLITWISE_USER_DIRECTORY_HPP
