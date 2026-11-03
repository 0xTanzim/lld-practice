#ifndef EXPONENTIAL_BACKOFF_HPP
#define EXPONENTIAL_BACKOFF_HPP

#include <algorithm>
#include "RetryPolicy.hpp"

class ExponentialBackoff : public RetryPolicy {
private:
    int baseMs;
    int factor;
    int maxAttemptsCount;
    int maxDelayMs;

public:
    ExponentialBackoff(int baseMs, int factor, int maxAttempts, int maxDelayMs = 5000)
        : baseMs(baseMs), factor(factor), maxAttemptsCount(maxAttempts), maxDelayMs(maxDelayMs) {}

    int maxAttempts() const override {
        return maxAttemptsCount;
    }

    int delayMs(int retryNumber) const override {
        long long delay = baseMs;
        for (int i = 1; i < retryNumber; ++i) {
            delay *= factor;
            if (delay >= maxDelayMs) {
                return maxDelayMs;
            }
        }
        return static_cast<int>(delay);
    }
};

#endif // EXPONENTIAL_BACKOFF_HPP
