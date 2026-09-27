# Notification System

LLD exercise: an observable notification pipeline — content decorated on the way out, fan-out to observers, delivery channels chosen per strategy.

## Patterns Used

| Pattern | Where | Idea |
|---|---|---|
| **Decorator** | `notifications/` | Wrap `INotification` — `TimestampDecorator`, `SignatureDecorator` stack behavior dynamically |
| **Observer** | `observers/` | `NotificationObservable` pushes updates to `Logger` + `NotificationEngine` on every new notification |
| **Strategy** | `strategies/` | Engine holds interchangeable delivery channels — Email, SMS, PopUp |
| **Singleton** | `service/NotificationService.h` | One service, single observable for all observers |

## Structure

```
notification-system/
├── main.cpp                      # Composition root
│
├── notifications/                # Decorator layer
│   ├── INotification.h           # Component interface
│   ├── SimpleNotification.h      # Concrete component
│   ├── INotificationDecorator.h  # Base decorator (owns wrapped)
│   ├── TimestampDecorator.h
│   └── SignatureDecorator.h
│
├── observers/                    # Observer layer
│   ├── IObserver.h
│   ├── IObservable.h
│   ├── NotificationObservable.h  # Subject — owns current notification
│   ├── Logger.h                  # Concrete observer
│   └── NotificationEngine.h      # Concrete observer → fan-out to strategies
│
├── strategies/                   # Strategy layer
│   ├── INotificationStrategy.h
│   ├── EmailStrategy.h
│   ├── SMSStrategy.h
│   └── PopUpStrategy.h
│
└── service/
    └── NotificationService.h     # Singleton facade
```

## Build & Run

```bash
g++ -std=c++26 -Wall -o main main.cpp
./main
```

## Flow

```
SimpleNotification
    → TimestampDecorator → SignatureDecorator   (content built up)
    → NotificationService::sendNotification
        → NotificationObservable::setNotification
            → notifyObservers()
                → Logger::update()              (log)
                → NotificationEngine::update()  (Email + SMS + PopUp)
```

## Ownership Rules

- `INotificationDecorator` owns and deletes its wrapped notification (chain frees itself)
- `NotificationObservable` owns the current notification (old one deleted on replace)
- `NotificationEngine` owns its strategies (deleted in dtor)
- Observers register in ctor, unregister in dtor — no dangling pointers

## Extending

- **New content behavior**: subclass `INotificationDecorator`, wrap around any notification
- **New observer**: subclass `IObserver`, `addObserver()` on the observable
- **New delivery channel**: subclass `INotificationStrategy`, `addNotificationStrategy()` on engine
