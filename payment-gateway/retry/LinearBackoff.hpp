#ifndef LINEAR_BACKOFF_HPP
#define LINEAR_BACKOFF_HPP

#include <algorithm>
#include "RetryPolicy.hpp"

class LinearBackoff : public RetryPolicy {
private:
    int baseMs;
    int stepMs;
    int maxAttemptsCount;
    int maxDelayMs;

public:
    LinearBackoff(int baseMs, int stepMs, int maxAttempts, int maxDelayMs = 2000)
        : baseMs(baseMs), stepMs(stepMs), maxAttemptsCount(maxAttempts), maxDelayMs(maxDelayMs) {}

    int maxAttempts() const override {
        return maxAttemptsCount;
    }

    int delayMs(int retryNumber) const override {
        return std::min(baseMs + (retryNumber - 1) * stepMs, maxDelayMs);
    }
};

#endif // LINEAR_BACKOFF_HPP
