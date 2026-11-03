#ifndef RETRY_GATEWAY_PROXY_HPP
#define RETRY_GATEWAY_PROXY_HPP

#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include "../gateways/PaymentGateway.hpp"
#include "../retry/RetryPolicy.hpp"
#include "../utils/ConsoleStyle.hpp"
using namespace std;

class RetryGatewayProxy : public PaymentGateway {
private:
    unique_ptr<PaymentGateway> realGateway;
    unique_ptr<RetryPolicy> retryPolicy;

public:
    RetryGatewayProxy(unique_ptr<PaymentGateway> gateway, unique_ptr<RetryPolicy> policy)
        : realGateway(move(gateway)), retryPolicy(move(policy)) {
        if (!realGateway || !retryPolicy) {
            throw invalid_argument("RetryGatewayProxy requires a gateway and a retry policy.");
        }
    }

    const char *name() const override {
        return realGateway->name();
    }

    PaymentResult processPayment(const PaymentRequest &request) override {
        const int maxAttempts = retryPolicy->maxAttempts();
        PaymentResult result{false, true, "not attempted"};

        for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
            if (attempt > 1) {
                int delay = retryPolicy->delayMs(attempt - 1);
                Console::warn("[proxy] attempt " + to_string(attempt - 1) + " failed — backing off "
                              + to_string(delay) + "ms");
                this_thread::sleep_for(chrono::milliseconds(delay));
            }

            Console::info("[proxy] attempt " + to_string(attempt) + "/" + to_string(maxAttempts));
            result = realGateway->processPayment(request);

            if (result.success) {
                return result;
            }
            if (!result.retryable) {
                Console::warn("[proxy] non-retryable failure — retries skipped");
                return result;
            }
        }

        Console::fail("[proxy] " + string(name()) + " failed after "
                      + to_string(maxAttempts) + " attempts");
        return result;
    }
};

#endif // RETRY_GATEWAY_PROXY_HPP
