#ifndef DATING_APP_ERROR_HPP
#define DATING_APP_ERROR_HPP

#include <stdexcept>
#include <string>

// Every way an operation can be refused. One type: the caller shows the message,
// it never branches on the class. The original printed "User not found." and
// carried on with a null pointer.
class AppError : public std::runtime_error {
public:
    explicit AppError(const std::string &message) : std::runtime_error(message) {}
};

#endif // DATING_APP_ERROR_HPP
