# Tomato — Online Food Ordering System

C++ low-level design (LLD) exercise: a food-ordering flow demonstrating SOLID class design and classic GoF design patterns.

## Features

- Search restaurants by location (case-insensitive)
- Build a cart for a selected restaurant
- Checkout as **Delivery** or **Pickup**
- Pluggable payment via Strategy pattern
- Order creation via Abstract Factory (immediate or scheduled)
- Singleton managers, static notification service

## Design Patterns

| Pattern | Where |
|---|---|
| Facade | `TomatoApp` — single entry point orchestrating the flow |
| Strategy | `PaymentStrategy` — swappable payment methods |
| Abstract Factory | `OrderFactory` — `NowOrderFactory` / `ScheduledOrderFactory` |
| Singleton | `RestaurantManager`, `OrderManager` |
| Template / Inheritance | `Order` ← `DeliveryOrder`, `PickupOrder` |

## Project Structure

```
tomato/
├── main.cpp                    # Composition root and entry point
├── TomatoApp.h                 # Facade class (main orchestrator)
│
├── models/
│   ├── MenuItem.h
│   ├── Restaurant.h
│   ├── User.h
│   ├── Cart.h
│   ├── Order.h                 # Abstract base
│   ├── DeliveryOrder.h
│   └── PickupOrder.h
│
├── managers/
│   ├── RestaurantManager.h
│   └── OrderManager.h
│
├── strategies/
│   ├── PaymentStrategy.h               # Abstract strategy
│   ├── CreditCardPaymentStrategy.h
│   ├── BkashPaymentStrategy.h
│   └── DutchBanglaPaymentStrategy.h
│
├── factories/
│   ├── OrderFactory.h          # Abstract factory
│   ├── NowOrderFactory.h
│   └── ScheduledOrderFactory.h
│
├── services/
│   └── NotificationService.h
│
└── utils/
    ├── TimeUtils.h
    └── Currency.h
```

## Build & Run

```bash
g++ -std=c++26 -Wall -o tomato main.cpp
./tomato
```

## Flow

```
searchRestaurants → selectRestaurant → addToCart
    → checkoutNow / checkoutScheduled → payForOrder → notification
```
