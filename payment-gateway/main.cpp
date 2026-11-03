#include "enums/Currency.hpp"
#include "enums/GatewayType.hpp"
#include "managers/GatewayFactory.hpp"
#include "managers/PaymentController.hpp"
#include "managers/PaymentService.hpp"
#include "models/PaymentRequest.hpp"
#include "models/Subscription.hpp"
#include "observers/PaymentLogger.hpp"
#include "subscription/RecurringBilling.hpp"
#include "utils/ConsoleStyle.hpp"
using namespace std;

int main() {
    Console::banner("PAYMENT GATEWAY — LLD DEMO");

    PaymentLogger logger;
    PaymentService::getInstance().events().addObserver(&logger);
    auto &controller = PaymentController::getInstance();

    Console::section("One-off payments (BD)");
    controller.handlePayment(GatewayType::BKASH,
                             PaymentRequest("Rahim Uddin", "Fatema Akter", 1500, Currency::BDT));

    // Dutch-Bangla rail fails once; linear backoff retries and succeeds.
    controller.handlePayment(GatewayType::DUTCH_BANGLA,
                             PaymentRequest("Tanvir Ahmed", "Nusrat Jahan", 800, Currency::BDT));

    Console::section("One-off payments (US)");

    // Stripe rail fails twice; exponential backoff succeeds on attempt 3.
    controller.handlePayment(GatewayType::STRIPE,
                             PaymentRequest("Alice Johnson", "Bob Smith", 250, Currency::USD));

    // PayPal is down; the proxy exhausts its retry budget.
    controller.handlePayment(GatewayType::PAYPAL,
                             PaymentRequest("Michael Brown", "Emma Davis", 99, Currency::USD));

    Console::section("Gateway validation (currency mismatch)");
    controller.handlePayment(GatewayType::STRIPE,
                             PaymentRequest("Rahim Uddin", "Fatema Akter", 500, Currency::BDT));

    Console::section("Recurring subscription — healthy gateway");
    Subscription streamSubscription("SUB-1001", "Nusrat Jahan", "StreamBD", 500, Currency::BDT, 30);
    auto bdGateway = GatewayFactory::getInstance().createGateway(GatewayType::BKASH);
    RecurringBilling billing;
    for (int cycle = 1; cycle <= 3; ++cycle) {
        billing.runCycle(streamSubscription, *bdGateway);
    }

    Console::section("Recurring subscription — failing gateway");
    Subscription cloudSubscription("SUB-2002", "John Carter", "CloudTunes", 20, Currency::USD, 30);
    auto failingGateway = GatewayFactory::getInstance().createGateway(GatewayType::PAYPAL);
    for (int cycle = 1; cycle <= 3; ++cycle) {
        billing.runCycle(cloudSubscription, *failingGateway);
    }

    Console::divider();
    Console::kv("Final state", cloudSubscription.getId() + " = "
                                   + cloudSubscription.getStateName());
    Console::divider();

    return 0;
}
