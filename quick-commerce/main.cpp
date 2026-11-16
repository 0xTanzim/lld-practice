#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "errors/CheckoutError.hpp"
#include "factories/ProductFactory.hpp"
#include "managers/CheckoutService.hpp"
#include "managers/OrderBook.hpp"
#include "models/Cart.hpp"
#include "models/Catalog.hpp"
#include "models/Customer.hpp"
#include "models/Inventory.hpp"
#include "models/Location.hpp"
#include "models/Order.hpp"
#include "models/Product.hpp"
#include "models/Rider.hpp"
#include "money/Money.hpp"
#include "payment/BkashWallet.hpp"
#include "payment/CardPayment.hpp"
#include "payment/CashOnDelivery.hpp"
#include "payment/PaymentMethod.hpp"
#include "promotions/Coupon.hpp"
#include "strategies/OrderAllocator.hpp"
#include "stores/DarkStore.hpp"
#include "stores/StoreNetwork.hpp"
#include "utils/ConsoleStyle.hpp"

using namespace std;

namespace {

constexpr double SERVICE_RADIUS_KM = 5.0;

constexpr size_t NAME_WIDTH = 18;
constexpr size_t QTY_WIDTH = 6;
constexpr size_t UNIT_WIDTH = 11;
constexpr size_t TOTAL_WIDTH = 13;

struct Checks {
    int passed = 0;
    int failed = 0;

    void expect(bool condition, const string &label) {
        if (condition) {
            Console::ok(label);
            ++passed;
        } else {
            Console::fail(label);
            ++failed;
        }
    }
};

// Riders start empty on purpose: the first checkout fails on rider
// availability after stock has already been lined up.
StoreNetwork buildNetwork() {
    StoreNetwork network;

    DarkStore dhanmondi("Dhanmondi", {0.0, 0.0});
    dhanmondi.inventory().stockIn(ProductFactory::APPLE, 70);
    dhanmondi.inventory().stockIn(ProductFactory::BANANA, 20);
    network.open(std::move(dhanmondi));

    DarkStore mirpur("Mirpur", {4.0, 1.0});
    mirpur.inventory().stockIn(ProductFactory::APPLE, 40);
    mirpur.inventory().stockIn(ProductFactory::CHOCOLATE, 10);
    network.open(std::move(mirpur));

    DarkStore gulshan("Gulshan", {2.0, 3.0});
    gulshan.inventory().stockIn(ProductFactory::BANANA, 5);
    gulshan.inventory().stockIn(ProductFactory::TSHIRT, 7);
    gulshan.inventory().stockIn(ProductFactory::JEANS, 3);
    network.open(std::move(gulshan));

    return network;
}

void hireRiders(StoreNetwork &network, const string &store, const vector<Rider> &riders) {
    DarkStore *target = network.find(store);
    if (target == nullptr) {
        throw out_of_range("demo: unknown store " + store);
    }
    for (const Rider &rider : riders) {
        target->riders().hire(rider);
    }
}

void addToCart(Cart &cart, const Catalog &catalog, int sku, int quantity) {
    cart.add(catalog.require(sku), quantity);
}

string formatKm(double km) {
    ostringstream out;
    out << fixed << setprecision(1) << km << " km";
    return out.str();
}

string formatPoint(const Location &point) {
    ostringstream out;
    out << fixed << setprecision(1) << "(" << point.x << ", " << point.y << ")";
    return out.str();
}

// Runs one checkout and reports a refusal instead of ending the demo.
bool tryCheckout(CheckoutService &service, const Customer &customer, const Cart &cart,
                 PaymentMethod &payment, const string &coupon, string &reason) {
    try {
        service.checkout(customer, cart, payment, coupon);
        return true;
    } catch (const CheckoutError &error) {
        reason = error.what();
        return false;
    }
}

void printCatalog(const Catalog &catalog) {
    Console::section("Catalog (product master, prices independent of stores)");
    for (int sku : {ProductFactory::APPLE, ProductFactory::BANANA, ProductFactory::CHOCOLATE,
                    ProductFactory::TSHIRT, ProductFactory::JEANS}) {
        const Product &product = catalog.require(sku);
        cout << "    " << Console::padRight(to_string(product.sku()), 6)
             << Console::padRight(product.name(), NAME_WIDTH)
             << Console::padLeft(product.price().format(), UNIT_WIDTH) << "\n";
    }
}

void printStore(const DarkStore &store, const Catalog &catalog) {
    cout << "    " << Console::BOLD << store.name() << Console::RESET << "  "
         << Console::DIM << formatPoint(store.location()) << Console::RESET << "  riders: "
         << store.riders().idleCount() << " idle · " << store.riders().availableCount() << "/"
         << store.riders().size() << " with spare capacity\n";
    for (int sku : store.inventory().stockedSkus()) {
        cout << "        " << Console::padRight(catalog.require(sku).name(), NAME_WIDTH)
             << Console::padRight("on hand " + to_string(store.inventory().onHand(sku)), 14)
             << "available " << store.inventory().available(sku) << "\n";
    }
}

void printStores(const StoreNetwork &network, const Catalog &catalog) {
    for (const DarkStore &store : network.stores()) {
        printStore(store, catalog);
    }
}

void printOrder(const Order &order) {
    cout << "\n" << Console::GREEN << "  Order #" << order.id() << Console::RESET << " for "
         << order.customerName() << "  " << Console::DIM << "("
         << (order.isPaid() ? "paid" : "payment on delivery") << ")" << Console::RESET << "\n";

    int index = 1;
    for (const OrderShipment &shipment : order.shipments()) {
        cout << "    " << Console::BOLD << "Shipment " << index++ << Console::RESET << " ← "
             << shipment.storeName << " · rider " << shipment.riderName << " · "
             << formatKm(shipment.distanceKm) << "\n";
        cout << "      " << Console::DIM << Console::padRight("Item", NAME_WIDTH)
             << Console::padRight("Qty", QTY_WIDTH) << Console::padLeft("Unit", UNIT_WIDTH)
             << Console::padLeft("Line", TOTAL_WIDTH) << Console::RESET << "\n";
        for (const OrderLine &line : shipment.lines) {
            cout << "      " << Console::padRight(line.productName, NAME_WIDTH)
                 << Console::padRight("x" + to_string(line.quantity), QTY_WIDTH)
                 << Console::padLeft(line.unitPrice.format(), UNIT_WIDTH)
                 << Console::padLeft(line.lineTotal().format(), TOTAL_WIDTH) << "\n";
        }
    }

    Console::divider(62);
    Console::kv("Items", order.summary().itemSubtotal().format());
    Console::kv("Coupon", order.summary().hasCoupon() ? order.summary().couponCode() : "—");
    Console::kv("Discount", order.summary().discount().isZero()
                                ? "—"
                                : "-" + order.summary().discount().format());
    Console::kv("Delivery", order.summary().deliveryFee().format());
    Console::kv("Total", Console::BOLD + order.summary().total().format() + Console::RESET);
    Console::kv("Payment", order.paymentMethodName() + " · " + order.paymentReference());
}

void printRejection(const string &reason) {
    Console::warn("Rejected as expected — " + reason);
}

void printAvailable(const StoreNetwork &network, const Catalog &catalog, int sku) {
    const Product &product = catalog.require(sku);
    cout << "    " << Console::DIM << product.name() << Console::RESET << " available → ";
    int total = 0;
    for (const DarkStore &store : network.stores()) {
        const int available = store.inventory().available(sku);
        total += available;
        cout << store.name() << " " << available << "  ";
    }
    cout << Console::BOLD << "(total " << total << ")" << Console::RESET << "\n";
}

}

int main() {
    Console::banner("QUICK COMMERCE — MULTI-STORE ORDER FULFILMENT");

    Checks checks;
    Catalog catalog = ProductFactory::buildCatalog();
    StoreNetwork network = buildNetwork();

    printCatalog(catalog);
    Console::section("Dark stores");
    printStores(network, catalog);

    OrderBook book;
    CheckoutService service(OrderAllocator(network, SERVICE_RADIUS_KM), book, Money::taka(30));
    service.addCoupon(Coupon("EID10", 1000, Money::taka(500)));

    Customer customer("Aditya Rahman", {1.0, 1.0});
    BkashWallet bkash("01711-000000", Money::taka(5000));
    CardPayment card("Visa Debit", "4242", Money::taka(800));
    CashOnDelivery cod;

    // ---------------------------------------------------------------- 1
    Console::section("1. One cart, two stores: 100 apples but the nearest store has 70");
    cout << "    Drop point: " << formatPoint(customer.location) << " — radius "
         << formatKm(SERVICE_RADIUS_KM) << "\n";
    printAvailable(network, catalog, ProductFactory::APPLE);

    Cart cart;
    addToCart(cart, catalog, ProductFactory::APPLE, 100);
    addToCart(cart, catalog, ProductFactory::BANANA, 3);
    addToCart(cart, catalog, ProductFactory::CHOCOLATE, 2);
    Console::info("Cart: " + to_string(cart.itemCount()) + " units, "
                  + to_string(cart.lineCount()) + " lines, " + cart.subtotal().format());
    Console::info("No single store holds all of it, so the cart has to be split");

    const int appleBefore = network.find("Dhanmondi")->inventory().available(ProductFactory::APPLE);
    string reason;
    const bool placedWithoutRiders = tryCheckout(service, customer, cart, bkash, "", reason);
    if (placedWithoutRiders) {
        Console::fail("expected the rider step to fail");
        checks.expect(false, "rider shortage is reported");
    } else {
        printRejection(reason);
        checks.expect(reason.find("no rider") != string::npos,
                      "checkout fails when no rider can be assigned");
    }
    checks.expect(network.find("Dhanmondi")->inventory().available(ProductFactory::APPLE)
                      == appleBefore,
                  "reserved stock is released after the failure (no inventory destroyed)");
    checks.expect(network.find("Dhanmondi")->inventory().reserved(ProductFactory::APPLE) == 0,
                  "no reservation is left dangling");

    // ---------------------------------------------------------------- 2
    Console::section("2. Hire riders, retry the same cart with coupon EID10");
    hireRiders(network, "Dhanmondi", {Rider("R-01", "Rakib"), Rider("R-02", "Sabbir")});
    hireRiders(network, "Mirpur", {Rider("R-11", "Nadia"), Rider("R-12", "Farhan")});
    hireRiders(network, "Gulshan", {Rider("R-21", "Imran")});
    Console::info("Checkout did not mutate the cart, so the retry is just a retry");

    Order order1 = service.checkout(customer, cart, bkash, "EID10");
    printOrder(order1);

    checks.expect(order1.shipmentCount() == 2, "cart split into 2 shipments");
    checks.expect(order1.summary().itemSubtotal() == Money::taka(2130),
                  "item subtotal is 100x20 + 3x10 + 2x50 = ৳2,130.00");
    checks.expect(order1.summary().discount() == Money::taka(213), "EID10 gives 10% = ৳213.00");
    checks.expect(order1.summary().deliveryFee() == Money::taka(60), "2 shipments x ৳30 delivery");
    checks.expect(order1.summary().total() == Money::taka(1977), "total is ৳1,977.00");
    checks.expect(network.find("Dhanmondi")->inventory().onHand(ProductFactory::APPLE) == 0,
                  "Dhanmondi shipped all 70 apples");
    checks.expect(network.find("Mirpur")->inventory().onHand(ProductFactory::APPLE) == 10,
                  "Mirpur shipped the remaining 30 apples");
    checks.expect(bkash.balance() == Money::taka(3023), "wallet debited ৳1,977.00");

    // ---------------------------------------------------------------- 3
    Console::section("3. A declined card must put stock and riders back");
    printAvailable(network, catalog, ProductFactory::APPLE);
    Cart cardCart;
    addToCart(cardCart, catalog, ProductFactory::APPLE, 10);
    addToCart(cardCart, catalog, ProductFactory::JEANS, 1);
    const bool cardPlaced = tryCheckout(service, customer, cardCart, card, "", reason);
    if (cardPlaced) {
        Console::fail("expected the card to be declined");
        checks.expect(false, "declined card is reported");
    } else {
        printRejection(reason);
        checks.expect(reason.find("available") != string::npos,
                      "declined card rejects the order and names the limit");
    }
    checks.expect(network.find("Mirpur")->inventory().onHand(ProductFactory::APPLE) == 10,
                  "Mirpur apples untouched after the decline");
    checks.expect(network.find("Gulshan")->inventory().onHand(ProductFactory::JEANS) == 3,
                  "Gulshan jeans untouched after the decline");
    checks.expect(network.find("Mirpur")->inventory().reserved(ProductFactory::APPLE) == 0,
                  "Mirpur reservation released after the decline");
    checks.expect(network.find("Mirpur")->riders().idleCount() == 1,
                  "Mirpur rider released after the decline");

    // ---------------------------------------------------------------- 4
    Console::section("4. Refusals that never touch stock");
    Cart smallCart;
    addToCart(smallCart, catalog, ProductFactory::APPLE, 2);

    const bool belowMinimum = tryCheckout(service, customer, smallCart, bkash, "EID10", reason);
    if (belowMinimum) {
        Console::fail("expected the coupon minimum to reject the order");
    } else {
        printRejection(reason);
        checks.expect(reason.find("minimum spend") != string::npos,
                      "coupon below minimum spend is rejected");
    }

    const bool unknownCoupon = tryCheckout(service, customer, smallCart, bkash, "NOPE", reason);
    if (unknownCoupon) {
        Console::fail("expected an unknown coupon to reject the order");
    } else {
        printRejection(reason);
        checks.expect(reason.find("unknown coupon") != string::npos,
                      "unknown coupon code is reported");
    }
    checks.expect(network.find("Mirpur")->inventory().reserved(ProductFactory::APPLE) == 0,
                  "no stock was reserved for the refused carts");

    Cart hugeCart;
    addToCart(hugeCart, catalog, ProductFactory::APPLE, 500);
    const bool filled = tryCheckout(service, customer, hugeCart, bkash, "", reason);
    if (filled) {
        Console::fail("expected a stock shortage");
    } else {
        printRejection(reason);
        checks.expect(reason.find("SKU 101 x490") != string::npos,
                      "shortage names the missing SKU and quantity");
    }

    Customer farAway("Far Away Customer", {50.0, 50.0});
    Cart farCart;
    addToCart(farCart, catalog, ProductFactory::APPLE, 1);
    const bool outOfRange = tryCheckout(service, farAway, farCart, bkash, "", reason);
    if (outOfRange) {
        Console::fail("expected no store in range");
    } else {
        printRejection(reason);
        checks.expect(reason.find("no dark store") != string::npos,
                      "drop point outside the service radius is rejected");
    }

    Cart emptyCart;
    checks.expect(!tryCheckout(service, customer, emptyCart, bkash, "", reason),
                  "empty cart is rejected");

    // ---------------------------------------------------------------- 5
    Console::section("5. Cash on delivery: accepted, not paid");
    Cart codCart;
    addToCart(codCart, catalog, ProductFactory::BANANA, 4);
    addToCart(codCart, catalog, ProductFactory::CHOCOLATE, 2);
    Order codOrder = service.checkout(customer, codCart, cod);
    printOrder(codOrder);
    checks.expect(!codOrder.isPaid(), "COD order is recorded as unpaid");
    checks.expect(book.capturedRevenue() == Money::taka(1977), "captured revenue is only order #1");
    checks.expect(book.pendingRevenue() == Money::taka(200),
                  "pending revenue is the COD order (৳140.00 items + ৳60.00 delivery)");

    // ---------------------------------------------------------------- 6
    Console::section("6. Final state");
    printStores(network, catalog);
    Console::kv("Orders recorded", to_string(book.size()));
    Console::kv("Captured", book.capturedRevenue().format());
    Console::kv("Pending (COD)", book.pendingRevenue().format());

    Console::divider(66);
    if (checks.failed == 0) {
        Console::ok(to_string(checks.passed) + " checks passed");
        return 0;
    }
    Console::fail(to_string(checks.failed) + " of " + to_string(checks.passed + checks.failed)
                  + " checks failed");
    return 1;
}
