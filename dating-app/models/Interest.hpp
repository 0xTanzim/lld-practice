#ifndef DATING_INTEREST_HPP
#define DATING_INTEREST_HPP

#include <stdexcept>
#include <string>
#include <utility>

// A value type. The original stored Interest* and deleted it by hand, so a
// UserProfile copy double-freed and a vector of them leaked on any throw.
class Interest {
public:
    Interest(std::string name, std::string category)
        : name_(std::move(name)), category_(std::move(category)) {
        if (name_.empty()) {
            throw std::invalid_argument("Interest: name must not be empty");
        }
        if (category_.empty()) {
            throw std::invalid_argument("Interest: category must not be empty");
        }
    }

    const std::string &name() const {
        return name_;
    }

    const std::string &category() const {
        return category_;
    }

private:
    std::string name_;
    std::string category_;
};

#endif // DATING_INTEREST_HPP
