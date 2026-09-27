#include <iostream>
#include "service/NotificationService.h"
#include "notifications/SimpleNotification.h"
#include "notifications/TimestampDecorator.h"
#include "notifications/SignatureDecorator.h"
#include "observers/Logger.h"
#include "observers/NotificationEngine.h"
#include "strategies/EmailStrategy.h"
#include "strategies/SMSStrategy.h"
#include "strategies/PopUpStrategy.h"
using namespace std;

int main() {
    NotificationService *service = NotificationService::getInstance();

    auto *logger = new Logger();
    auto *engine = new NotificationEngine();

    engine->addNotificationStrategy(new EmailStrategy("random.person@gmail.com"));
    engine->addNotificationStrategy(new SMSStrategy("+880 1712345678"));
    engine->addNotificationStrategy(new PopUpStrategy());

    INotification *notification = new SimpleNotification("Your order has been shipped!");
    notification = new TimestampDecorator(notification);
    notification = new SignatureDecorator(notification, "Customer Care");

    service->sendNotification(notification);

    delete engine;
    delete logger;
    delete service;

    return 0;
}
