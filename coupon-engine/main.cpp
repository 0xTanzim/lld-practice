#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "checkout/CheckoutService.hpp"
#include "coupons/BankingCoupon.hpp"
#include "coupons/BulkPurchaseDiscount.hpp"
#include "coupons/FlashSaleCoupon.hpp"
#include "coupons/LoyaltyDiscount.hpp"
#include "coupons/SeasonalOffer.hpp"
#include "enums/ProductCategory.hpp"
#include "models/Cart.hpp"
#include "models/Catalog.hpp"
#include "models/Customer.hpp"
#include "models/Product.hpp"
#include "money/Money.hpp"
#include "utils/ConsoleStyle.hpp"

using namespace std;

namespace {

constexpr size_t ITEM_WIDTH = 18;
constexpr size_t CATEGORY_WIDTH = 13;
constexpr size_t QTY_WIDTH = 5;
constexpr size_t AMOUNT_WIDTH = 14;

CheckoutService buildBdCampaign() {
    CheckoutService service;
    service.addCoupon(make_unique<SeasonalOffer>(ProductCategory::CLOTHING, 1000));
    service.addCoupon(make_unique<LoyaltyDiscount>(500));
    service.addCoupon(make_unique<BulkPurchaseDiscount>(
        Money::fromMajor(1000, Currency::BDT), Money::fromMajor(100, Currency::BDT)));
    service.addCoupon(make_unique<BankingCoupon>(
        "bKash", Money::fromMajor(2000, Currency::BDT), 1500, Money::fromMajor(500, Currency::BDT)));
    return service;
}

CheckoutService buildUsCampaign() {
    CheckoutService service;
    service.addCoupon(make_unique<FlashSaleCoupon>(1000));
    service.addCoupon(make_unique<SeasonalOffer>(ProductCategory::CLOTHING, 1000));
    service.addCoupon(make_unique<BulkPurchaseDiscount>(
        Money::fromMajor(500, Currency::USD), Money::fromMajor(25, Currency::USD)));
    service.addCoupon(make_unique<BankingCoupon>(
        "Chase", Money::fromMajor(1000, Currency::USD), 1200, Money::fromMajor(40, Currency::USD)));
    return service;
}

void printCart(const Cart &cart) {
    Console::kv("Customer", cart.customer().name);
    Console::kv("Currency", toString(cart.currency()));
    Console::kv("Payment bank", cart.customer().paymentBank.empty() ? "—" : cart.customer().paymentBank);
    Console::kv("Loyalty", cart.customer().loyaltyMember ? "yes" : "no");

    cout << "\n    " << Console::BOLD
         << Console::padRight("Item", ITEM_WIDTH)
         << Console::padRight("Category", CATEGORY_WIDTH)
         << Console::padRight("Qty", QTY_WIDTH)
         << Console::padLeft("Line total", AMOUNT_WIDTH)
         << Console::RESET << "\n";
    cout << "    " << Console::DIM
         << Console::repeat("─", ITEM_WIDTH + CATEGORY_WIDTH + QTY_WIDTH + AMOUNT_WIDTH)
         << Console::RESET << "\n";

    for (const CartItem &item : cart.items()) {
        cout << "    " << Console::padRight(item.product().name(), ITEM_WIDTH)
             << Console::padRight(toString(item.product().category()), CATEGORY_WIDTH)
             << Console::padRight(to_string(item.quantity()), QTY_WIDTH)
             << Console::padLeft(item.subtotal().format(), AMOUNT_WIDTH) << "\n";
    }
}

void printReceipt(const CheckoutReceipt &receipt) {
    Console::divider();
    if (receipt.discounts.empty()) {
        Console::warn("No coupon applied");
    }
    for (const DiscountApplication &discount : receipt.discounts) {
        cout << "    " << Console::GREEN << "−" << Console::padLeft(discount.amount.format(), 12)
             << Console::RESET << "  " << discount.coupon << "\n";
    }
    Console::divider();
    Console::kv("Original", receipt.original.format());
    Console::kv("Saved", "-" + receipt.saved().format());
    Console::kv("Payable", Console::BOLD + receipt.finalTotal.format() + Console::RESET);
}

void runCheckout(const string &title, const CheckoutService &service, const Cart &cart) {
    Console::section(title);
    printCart(cart);
    try {
        const vector<string> candidates = service.preview(cart);
        Console::info("Preview: " + to_string(candidates.size()) + " rule-eligible coupon(s)");
        for (const string &name : candidates) {
            cout << "      " << Console::DIM << "•" << Console::RESET << " " << name << "\n";
        }
        printReceipt(service.checkout(cart));
    } catch (const exception &error) {
        Console::fail(string("Checkout rejected: ") + error.what());
    }
}

template <typename Action>
void expectFailure(const string &label, Action action) {
    try {
        action();
        Console::fail(label + " — no error raised");
    } catch (const exception &error) {
        Console::ok(label + " — " + error.what());
    }
}

}

int main() {
    Console::banner("COUPON ENGINE — STACKED CART DISCOUNTS");

    Console::section("1. Price lock and line ownership");
    Customer rahim("Rahim Uddin", false, "bKash");
    Catalog catalog;
    catalog.add(Product("Winter Jacket", ProductCategory::CLOTHING,
                        Money::fromMajor(1000, Currency::BDT)));

    Cart probe(Currency::BDT, rahim);
    probe.add(catalog.require("Winter Jacket"), 1);
    Console::kv("Line at add time", probe.items().front().subtotal().format());

    catalog.reprice("Winter Jacket", Money::fromMajor(2000, Currency::BDT));
    Console::kv("Catalog price now", catalog.require("Winter Jacket").price().format());
    Console::kv("Cart line still", probe.items().front().subtotal().format());

    probe.add(catalog.require("Winter Jacket"), 2);
    probe.add(catalog.require("Winter Jacket"), 1);
    Console::kv("Lines in cart", to_string(probe.items().size()));
    for (const CartItem &item : probe.items()) {
        Console::kv(item.product().name(),
                    to_string(item.quantity()) + " × " + item.unitPrice().format());
    }
    Console::kv("Probe total", probe.originalTotal().format());
    Console::info("A catalog reprice never rewrites a line already in the cart");

    Console::section("2. Dhaka cart — stacked coupons");
    Customer tanvir("Tanvir Ahmed", true, "bKash");
    Cart bdCart(Currency::BDT, tanvir);
    bdCart.add(Product("Winter Jacket", ProductCategory::CLOTHING, Money::fromMajor(1000, Currency::BDT)), 1);
    bdCart.add(Product("Smartphone", ProductCategory::ELECTRONICS, Money::fromMajor(20000, Currency::BDT)), 1);
    bdCart.add(Product("Jeans", ProductCategory::CLOTHING, Money::fromMajor(1000, Currency::BDT)), 2);
    bdCart.add(Product("Headphones", ProductCategory::ELECTRONICS, Money::fromMajor(2000, Currency::BDT)), 1);

    const CheckoutService bdCampaign = buildBdCampaign();
    Console::info("Registered coupons: " + to_string(bdCampaign.couponCount()));
    runCheckout("Checkout — bKash campaign", bdCampaign, bdCart);

    Console::section("3. New York cart — exclusive flash sale stops the chain");
    Customer alice("Alice Johnson", false, "Chase");
    Cart usCart(Currency::USD, alice);
    usCart.add(Product("Wool Coat", ProductCategory::CLOTHING, Money::fromMajor(120, Currency::USD)), 1);
    usCart.add(Product("Smartphone", ProductCategory::ELECTRONICS, Money::fromMajor(800, Currency::USD)), 1);
    usCart.add(Product("Headphones", ProductCategory::ELECTRONICS, Money::fromMajor(150, Currency::USD)), 1);

    const CheckoutService usCampaign = buildUsCampaign();
    runCheckout("Checkout — Chase campaign", usCampaign, usCart);
    Console::info("Preview lists rule-eligible coupons; the non-combinable flash sale ends the chain");

    Console::section("4. Edge cases");
    expectFailure("quantity 0", [&] {
        bdCart.add(Product("Jeans", ProductCategory::CLOTHING,
                           Money::fromMajor(1000, Currency::BDT)), 0);
    });

    Cart emptyCart(Currency::BDT, rahim);
    expectFailure("empty cart checkout", [&] { bdCampaign.checkout(emptyCart); });

    expectFailure("USD cart through BDT-configured campaign", [&] { bdCampaign.checkout(usCart); });

    const CheckoutReceipt first = bdCampaign.checkout(bdCart);
    const CheckoutReceipt second = bdCampaign.checkout(bdCart);
    Console::kv("First payable", first.finalTotal.format());
    Console::kv("Second payable", second.finalTotal.format());
    Console::kv("Cart discounts", bdCart.discountTotal().format());
    Console::info("Checkout works on a copy: repeating it is idempotent and the cart stays clean");

    Console::divider();
    return 0;
}
