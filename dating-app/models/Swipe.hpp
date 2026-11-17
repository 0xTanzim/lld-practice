#ifndef DATING_SWIPE_HPP
#define DATING_SWIPE_HPP

#include <stdexcept>
#include <string>

enum class SwipeAction {
    LEFT,  // pass
    RIGHT  // like
};

inline std::string toString(SwipeAction action) {
    switch (action) {
    case SwipeAction::LEFT:
        return "left";
    case SwipeAction::RIGHT:
        return "right";
    }
    throw std::logic_error("SwipeAction: unhandled enumerator");
}

#endif // DATING_SWIPE_HPP
