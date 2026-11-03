#ifndef GATEWAY_FACTORY_HPP
#define GATEWAY_FACTORY_HPP

#include <memory>
#include <stdexcept>
#include "../enums/GatewayType.hpp"
#include "../gateways/BkashGateway.hpp"
#include "../gateways/DutchBanglaGateway.hpp"
#include "../gateways/PayPalGateway.hpp"
#include "../gateways/StripeGateway.hpp"
#include "../proxy/RetryGatewayProxy.hpp"
#include "../retry/ExponentialBackoff.hpp"
#include "../retry/LinearBackoff.hpp"
using namespace std;

class GatewayFactory {
public:
    static GatewayFactory &getInstance() {
        static GatewayFactory instance;
        return instance;
    }

    unique_ptr<PaymentGateway> createGateway(GatewayType type) const {
        switch (type) {
        case GatewayType::BKASH:
            return wrap(make_unique<BkashGateway>(), make_unique<ExponentialBackoff>(100, 2, 3));
        case GatewayType::DUTCH_BANGLA:
            return wrap(make_unique<DutchBanglaGateway>(), make_unique<LinearBackoff>(100, 100, 3));
        case GatewayType::STRIPE:
            return wrap(make_unique<StripeGateway>(), make_unique<ExponentialBackoff>(150, 2, 3));
        case GatewayType::PAYPAL:
            return wrap(make_unique<PayPalGateway>(), make_unique<LinearBackoff>(100, 50, 2));
        }
        throw invalid_argument("Unknown gateway type.");
    }

private:
    GatewayFactory() = default;
    GatewayFactory(const GatewayFactory &) = delete;
    GatewayFactory &operator=(const GatewayFactory &) = delete;

    static unique_ptr<PaymentGateway> wrap(unique_ptr<PaymentGateway> gateway,
                                           unique_ptr<RetryPolicy> policy) {
        return make_unique<RetryGatewayProxy>(move(gateway), move(policy));
    }
};

#endif // GATEWAY_FACTORY_HPP
