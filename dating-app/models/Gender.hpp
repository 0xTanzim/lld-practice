#ifndef DATING_GENDER_HPP
#define DATING_GENDER_HPP

#include <stdexcept>
#include <string>

enum class Gender {
    MALE,
    FEMALE,
    NON_BINARY,
    OTHER
};

inline std::string toString(Gender gender) {
    switch (gender) {
    case Gender::MALE:
        return "Male";
    case Gender::FEMALE:
        return "Female";
    case Gender::NON_BINARY:
        return "Non-binary";
    case Gender::OTHER:
        return "Other";
    }
    throw std::logic_error("Gender: unhandled enumerator");
}

#endif // DATING_GENDER_HPP
