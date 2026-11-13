# Coupon Engine

LLD exercise: shopping cart with a coupon chain that stacks rule-based discounts, plus the money and ownership rules that keep cart totals honest.

Source started as a single-file snippet (`temp.tmp`); this version splits it into layers, removes the raw-pointer ownership model, and fixes the defects listed under [Fixes](#fixes-vs-the-original-snippet).

## Patterns Used

| Pattern | Where | Idea |
|---|---|---|
| **Chain of Responsibility** | `coupons/Coupon.hpp` | Coupons run in registration order; a non-combinable coupon ends the chain once it reduces the total |
| **Strategy** | `strategies/` | Swappable discount math: flat, percentage, capped percentage |
| **Template Method** | `Coupon::process` | Fixed flow: applicability → amount → clamp → apply → continue/stop |
| **Factory** | `factories/DiscountStrategyFactory.hpp` | One place that validates and builds discount strategies |
| **Facade** | `checkout/CheckoutService.hpp` | Registration, preview, and checkout behind one entry point |

No process-wide singleton: the coupon registry is per `CheckoutService`, so two carts can never share campaign state.

## Structure

```
coupon-engine/
├── main.cpp                        # Composition root + deterministic demo
│
├── enums/
│   ├── Currency.hpp                # BDT, USD
│   └── ProductCategory.hpp         # CLOTHING, ELECTRONICS, GROCERY
│
├── money/
│   └── Money.hpp                   # int64 minor units + checked arithmetic
│
├── models/
│   ├── Customer.hpp                # Name, loyalty flag, payment bank
│   ├── Product.hpp                 # Immutable price snapshot source
│   ├── Catalog.hpp                 # Stand-in product service (can reprice)
│   ├── CartItem.hpp                # Product owned by value + quantity
│   └── Cart.hpp                    # Totals derived from items, discount ledger
│
├── strategies/                     # STRATEGY
│   ├── DiscountStrategy.hpp
│   ├── FlatDiscount.hpp
│   ├── PercentageDiscount.hpp
│   └── CappedPercentageDiscount.hpp
│
├── factories/
│   └── DiscountStrategyFactory.hpp # FACTORY
│
├── coupons/                        # CHAIN OF RESPONSIBILITY
│   ├── Coupon.hpp                  # Template method + next link
│   ├── DiscountApplication.hpp     # One receipt line
│   ├── SeasonalOffer.hpp           # % off a product category
│   ├── LoyaltyDiscount.hpp         # % off cart for members
│   ├── BulkPurchaseDiscount.hpp    # Flat amount over a threshold
│   ├── BankingCoupon.hpp           # Card/bank % with a cap
│   └── FlashSaleCoupon.hpp         # Exclusive, stops the chain
│
├── managers/
│   └── CouponRegistry.hpp          # Owns the chain for one campaign
│
├── checkout/
│   ├── CheckoutReceipt.hpp         # Applies discounts, verifies the ledger
│   └── CheckoutService.hpp         # FACADE over a campaign
│
└── utils/
    ├── Format.hpp                  # Percent text
    └── ConsoleStyle.hpp            # Colored, UTF-8-aligned output
```

## Build & Run

```bash
g++ -std=c++26 -Wall -Wextra -o main main.cpp && ./main
```

## Flow

```
CheckoutService::checkout(cart)
    → copy the cart                          # caller's cart is never mutated
    → CouponRegistry::applyAll(copy)
        → Coupon::process                    # template method, per link
            → isApplicable?  (rule on cart)
            → discountFor?  (Strategy)
            → clamp to remaining total
            → Cart::applyDiscount (ledger)
            → continue unless exclusive and actually applied
    → CheckoutReceipt (verified: Σ discounts == original − payable)
```

## Price Ownership — the cart price-fetch issue

The snippet stored `Product*` in `CartItem` and also cached `originalTotal` when the item was added. Those two facts came from different moments in time:

```cpp
// original
void addProduct(Product* prod, int qty) {
    CartItem* item = new CartItem(prod, qty);
    items.push_back(item);
    originalTotal += item->itemTotal();   // price read now, cached forever
}
double itemTotal() { return product->getPrice() * quantity; }  // price read again, later
```

So a catalog reprice produced a cart whose line totals and cached total disagreed, and deleting a `Product` before the cart dangled the pointer. `main` in the original deletes `p1..p4` while the cart still references them.

This version resolves it with three rules:

1. **`CartItem` owns a `Product` by value.** The unit price is locked when the item enters the cart. Later catalog repricing cannot rewrite an existing line.
2. **The cart stores no total.** `originalTotal()` sums the line items on every read, so there is one source of truth.
3. **A repriced product is a different line identity.** Merging only happens when name, category, and unit price all match, so the cart never silently rewrites an existing line's price.

Trade-off: a cart cannot apply a live price change without re-adding the item. That is deliberate — the customer's total must not move under them — and a live repricing flow would be an explicit operation, not an implicit one.

## Delivery of discounts

The chain runs in registration order and each coupon sees the total left by earlier coupons:

| Order | Coupon | Base | Amount |
|---|---|---|---|
| 1 | Seasonal 10% off Clothing | Clothing subtotal ৳3,000 | ৳300 |
| 2 | Loyalty 5% off cart | Remaining ৳24,700 | ৳1,235 |
| 3 | Bulk ৳100 over ৳1,000 | Remaining ৳23,465 | ৳100 |
| 4 | bKash 15% capped ৳500 | Remaining ৳23,365 | ৳500 (cap binds) |

Result: ৳25,000 → ৳22,865.

Order is campaign configuration, not accidental behavior — the registry chain defines it, and stacking percentage-on-reduced-total is documented instead of hidden.

## Edge cases handled

| Case | Behavior |
|---|---|
| Quantity ≤ 0, negative price | rejected at construction |
| `price × quantity` overflow, sum overflow | `overflow_error`, never a wrapped total |
| Product in a different currency than the cart | rejected at `Cart::add` |
| Same product added twice at the same price | merged into one line; quantity checked for `int` overflow |
| Same product at a new price | separate line; the old line keeps its snapshot |
| Discount larger than the total | payable floors at zero, ledger records the clamped amount |
| Discount negative or in the wrong currency | rejected |
| Coupon computes zero (0%, cap 0) | skipped, chain continues — an exclusive coupon cannot starve later rules for free |
| Exclusive coupon with a real discount | chain stops |
| Empty cart checkout | rejected |
| Checkout invoked twice | identical receipt; the caller's cart is untouched |
| Discount ledger disagrees with payable | `logic_error` from `CheckoutReceipt` |

`preview()` reports coupons whose rules match the current cart. `checkout()` is authoritative: an exclusive coupon applied earlier can leave previewed coupons unapplied.

## Fixes vs the original snippet

| Original | Problem | Fix |
|---|---|---|
| `BankingCoupon`: `this->offCap = percent;` | the cap silently became the percentage, so a 15%/৳500 offer capped at ৳15 | cap stored from its own constructor parameter |
| `Product*` inside `CartItem` | dangling product, shared mutable price | `Product` owned by value |
| cached `originalTotal` + live `itemTotal()` | two totals that diverge after a reprice | totals derived from items |
| `applyDiscount(double)` mutating `currentTotal` | no receipt, repeat application double-subtracts, 0-floored overflow invisible | discount ledger on a cart copy, actual applied amount recorded |
| `new Product`, `new Cart`, `new CartItem`, never freed | leaks | value semantics + `vector<CartItem>` |
| lazily-created singletons | leaks, unsynchronized lazy init | removed; registry owned per service |
| `double` money | rounding drift | `int64` minor units, checked arithmetic |
| no input validation | nonsense states accepted | constructors and cart operations reject invalid state |

## Demo Notes

Deterministic, no randomness:

- **Dhaka cart** (Tanvir Ahmed, bKash, loyalty): 4 stacked coupons, cap binds.
- **New York cart** (Alice Johnson, Chase): exclusive flash sale applies and stops the chain — 4 rule-eligible previews, 1 applied.
- **Edge cases**: zero quantity, empty cart, BDT-configured campaign on a USD cart, repeated checkout.

## Extending

- **New coupon**: subclass `Coupon`, implement `isApplicable` / `discountFor` / `name`, register on the campaign
- **New discount math**: implement `DiscountStrategy`, add a factory method
- **New cart rule**: keep the rule in the coupon/cart layer, not in `main.cpp`
