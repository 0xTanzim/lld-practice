#ifndef RETRY_POLICY_HPP
#define RETRY_POLICY_HPP

class RetryPolicy {
public:
    virtual ~RetryPolicy() = default;
    virtual int maxAttempts() const = 0;
    // retryNumber is 1-based: the wait before the 2nd attempt is 1.
    virtual int delayMs(int retryNumber) const = 0;
};

#endif // RETRY_POLICY_HPP
