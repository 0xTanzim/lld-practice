#ifndef SPLITWISE_SPLIT_TYPE_HPP
#define SPLITWISE_SPLIT_TYPE_HPP

#include <string>

enum class SplitType { Equal, Exact, Percentage };

inline std::string name(SplitType type) {
    switch (type) {
        case SplitType::Equal:
            return "equal";
        case SplitType::Exact:
            return "exact";
        case SplitType::Percentage:
            return "percentage";
    }
    return "unknown";
}

#endif // SPLITWISE_SPLIT_TYPE_HPP
