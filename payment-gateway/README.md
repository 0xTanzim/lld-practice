# Payment Gateway

LLD exercise: multi-rail payment gateway with retry proxy, backoff strategies, event notifications, and recurring subscriptions.

## Patterns Used

| Pattern | Where | Idea |
|---|---|---|
| **Template Method** | `gateways/AbstractGateway.hpp` | Fixed flow: validate → initiate → confirm; subclasses supply only the steps |
| **Strategy** | `banking/`, `retry/` | Swappable payment rails; swappable backoff (`LinearBackoff`, `ExponentialBackoff`) |
| **Proxy** | `proxy/RetryGatewayProxy.hpp` | Wraps any gateway, adds retries around `processPayment` without touching the gateway |
| **Factory** | `managers/GatewayFactory.hpp` | Builds gateway + retry policy pairs per `GatewayType` |
| **Observer** | `observers/` | `PaymentService` publishes success/failure events; `PaymentLogger` reacts |
| **State** | `subscription/SubscriptionStates.hpp` | Subscription lifecycle: `ACTIVE` → `PAST_DUE` → `CANCELLED` (and recovery) |
| **Singleton** | `PaymentService`, `PaymentController`, `GatewayFactory` | One service/controller/factory instance (Meyers singletons) |

## Structure

```
payment-gateway/
├── main.cpp                        # Composition root + demo
│
├── enums/
│   ├── Currency.hpp                # USD, BDT
│   └── GatewayType.hpp             # BKASH, DUTCH_BANGLA, STRIPE, PAYPAL
│
├── models/
│   ├── PaymentRequest.hpp
│   └── Subscription.hpp            # Holds current state object
│
├── banking/                        # STRATEGY: payment rails
│   ├── BankingSystem.hpp
│   ├── BkashBanking.hpp
│   ├── DutchBanglaBanking.hpp
│   ├── StripeBanking.hpp
│   └── PayPalBanking.hpp
│
├── gateways/                       # TEMPLATE METHOD
│   ├── PaymentGateway.hpp          # Interface
│   ├── AbstractGateway.hpp         # Template method
│   ├── BkashGateway.hpp            # BD — BDT only
│   ├── DutchBanglaGateway.hpp      # BD — BDT only
│   ├── StripeGateway.hpp           # US — USD only
│   └── PayPalGateway.hpp           # US — USD only
│
├── retry/                          # STRATEGY: backoff policies
│   ├── RetryPolicy.hpp
│   ├── LinearBackoff.hpp
│   └── ExponentialBackoff.hpp
│
├── proxy/
│   └── RetryGatewayProxy.hpp       # PROXY: retry wrapper
│
├── subscription/
│   ├── ISubscriptionState.hpp
│   ├── SubscriptionStates.hpp      # STATE: ACTIVE / PAST_DUE / CANCELLED
│   └── RecurringBilling.hpp        # Runs one billing cycle
│
├── observers/                      # OBSERVER
│   ├── IPaymentObserver.hpp
│   ├── PaymentEventBus.hpp
│   └── PaymentLogger.hpp
│
├── managers/
│   ├── GatewayFactory.hpp
│   ├── PaymentService.hpp
│   └── PaymentController.hpp
│
└── utils/
    ├── Money.hpp                   # Currency formatting ($, ৳)
    └── ConsoleStyle.hpp            # Colored, aligned output
```

## Build & Run

```bash
g++ -std=c++26 -Wall -o main main.cpp && ./main
```

## Flow

```
PaymentController::handlePayment(type, request)
    → GatewayFactory::createGateway(type)        # gateway wrapped in RetryGatewayProxy
    → PaymentService::processPayment
        → RetryGatewayProxy::processPayment       # attempts + backoff
            → AbstractGateway::processPayment     # template method
                → validate → initiate → confirm
        → PaymentEventBus::publish → PaymentLogger
```

## Retry Policies

| Gateway | Policy | Attempts | Delays |
|---|---|---|---|
| bKash | Exponential (100ms, ×2) | 3 | 100, 200 |
| Dutch-Bangla | Linear (100ms + 100ms) | 3 | 100, 200 |
| Stripe | Exponential (150ms, ×2) | 3 | 150, 300 |
| PayPal | Linear (100ms + 50ms) | 2 | 100 |

## Subscription States

```
ACTIVE --failure(1)--> ACTIVE
ACTIVE --failure(2)--> PAST_DUE --failure--> CANCELLED
PAST_DUE --success--> ACTIVE
CANCELLED: canBill() == false, terminal
```

## Demo Notes

Banking rails are scripted (not random) so output is reproducible:

- bKash — always succeeds
- Dutch-Bangla — fails 1st attempt, succeeds on retry (linear backoff)
- Stripe — fails twice, succeeds on 3rd (exponential backoff)
- PayPal — always fails (retry exhaustion)

## Extending

- **New gateway**: add `XBanking` (Strategy rail) + `XGateway` (Template Method) + enum + factory case
- **New backoff**: implement `RetryPolicy`, wire in factory
- **New observer**: implement `IPaymentObserver`, register on `PaymentService::events()`
