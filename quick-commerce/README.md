# Quick Commerce — Multi-Store Order Fulfilment

LLD exercise: one order, several dark stores, riders, payment and coupons — with the invariants that keep stock and money honest when a step fails.

Starting point was a 714-line single-file snippet (`temp.tmp`). This version keeps the same domain, fixes the defects listed below, and uses only patterns already practiced elsewhere in this repo.

## Patterns Used

| Pattern | Where | What it does here |
|---|---|---|
| **Facade** | `managers/CheckoutService.hpp` | One `checkout()` call hides allocate → price → reserve → riders → charge → commit |
| **Template Method** | `CheckoutService::checkout` | The step *order* is fixed in one method; the variable steps are handed in |
| **Strategy** | `payment/PaymentMethod.hpp` | Three rails (`bKash`, card, COD) behind one `charge()`; checkout does not know which |
| **Factory** | `factories/ProductFactory.hpp` | Every SKU is built in one place; an unknown code throws instead of inventing a product |

That is the whole list. Buttons that exist only once are not patterns, they are just classes.

### What was deliberately left out

Earlier drafts of this project had more machinery than the problem needs. Removed:

| Removed | Why |
|---|---|
| Abstract `Allocator` + `unique_ptr` | There is exactly one allocation policy. An interface with one implementation is indirection, not a Strategy. |
| `CheckoutResult` (optional-based result type) | A refusal is exceptional, so it is an exception. `CheckoutResult` made every call site check a flag. |
| Five exception subclasses | No caller ever acted differently on one refusal than another. One `CheckoutError` with a message is enough. |
| RAII reservation guards | Two classes with move constructors to undo four lines of `reserve`. An explicit `catch` that releases what it took says the same thing and is readable top to bottom. |
| `CouponBook` | A lookup over one `vector<Coupon>`. The service owns the vector. |
| **Singleton** | Order ids live in one injected `OrderBook`. A global would make two carts share a sequence, and it is untestable. |
| **Chain of Responsibility** for allocation | See below — the request is split, not handed to one handler. |
| Registry / Value Object labels | Those are descriptions, not patterns this project needs to prove. |

## Why allocation is not a Chain of Responsibility

A chain answers "which single handler deals with this request?" Allocation answers a different question: **which stores each contribute a slice, and how much each gives.**

- The customer orders 100 apples; the nearest store has 70. The correct answer is 70 + 30, so the algorithm must accumulate a remainder.
- If the chain stops at the first store that can partially fill, it silently under-delivers.
- If the chain never stops, it is not a chain — it is a loop over stores with an accumulator.

There is also nowhere in a chain to share the per-SKU arithmetic, and no point where the order as a whole can still be refused.

So allocation is a distance-ordered greedy pass in `strategies/OrderAllocator.hpp`:
pass 1 uses a single store if it covers the cart; pass 2 splits nearest-first.

Chain of Responsibility *is* the right pattern when the semantics really are "run these rules in order until one decides" — see [`coupon-engine/`](../coupon-engine/). It would also fit a serviceability/fraud gate in front of checkout (`address serviceable? → minimum order? → account allowed?`).

## Structure

```
quick-commerce/
├── main.cpp                          # Composition root + deterministic demo (25 self-checks)
│
├── money/
│   └── Money.hpp                     # int64 poisha, checked arithmetic, ৳ formatting
│
├── models/
│   ├── Location.hpp                  # Planar km coordinates + distance
│   ├── Product.hpp                   # SKU, name, price (immutable value)
│   ├── Catalog.hpp                   # Product master — only source of names/prices
│   ├── Inventory.hpp                 # onHand / reserved / available per SKU, per store
│   ├── Customer.hpp                  # Name + drop location
│   ├── Cart.hpp                      # Price-locked lines, derived subtotal
│   ├── Rider.hpp                     # Id, name, concurrent-order capacity
│   └── Order.hpp                     # CheckoutSummary, OrderLine, OrderShipment, Order
│
├── stores/
│   ├── DarkStore.hpp                 # Inventory + riders for one fulfilment centre
│   └── StoreNetwork.hpp              # Ownership + "stores within radius, nearest first"
│
├── strategies/
│   ├── Allocation.hpp                # ShipmentPlan / AllocatedLine / Allocation
│   └── OrderAllocator.hpp            # Single-store-first, then greedy split
│
├── payment/                          # STRATEGY (rail per order)
│   ├── PaymentMethod.hpp             # Interface + PaymentResult (captured / pending)
│   ├── BkashWallet.hpp               # Balance-limited mobile wallet
│   ├── CardPayment.hpp               # Credit-line card, declines when it cannot cover
│   └── CashOnDelivery.hpp            # Accepted but not captured
│
├── promotions/
│   └── Coupon.hpp                    # Code, basis points, minimum spend
│
├── managers/
│   ├── RiderPool.hpp                 # Least-loaded dispatch, capacity-aware
│   ├── CheckoutService.hpp           # FACADE + fixed checkout pipeline
│   └── OrderBook.hpp                 # Accepted orders, id allocation, revenue split
│
├── factories/
│   └── ProductFactory.hpp            # FACTORY: the product master, unknown SKU throws
│
├── errors/
│   └── CheckoutError.hpp             # One refusal type
│
└── utils/
    └── ConsoleStyle.hpp              # Colored, UTF-8-width-aware output
```

## Build & Run

```bash
g++ -std=c++26 -Wall -Wextra -o main main.cpp && ./main
```

No dependencies, no build system, single translation unit. The demo is deterministic: no randomness, no wall clock, no network.

## Flow

```
CheckoutService::checkout(customer, cart, payment, couponCode)
    │
    ├─ 1. allocate(cart, drop)          # pure planning: no side effects
    ├─ 2. price(allocation, coupon)     # pure: no side effects
    │      └─ bad coupon or unfillable cart throws here, stock untouched
    │
    ├─ 3. reserve stock per shipment    # first side effect
    ├─ 4. assign one rider per shipment # may refuse: store is short-staffed
    ├─ 5. payment.charge(total)         # may refuse: declined / low balance
    ├─ 6. commit reservations           # from here the order must be recorded
    └─ 7. Order recorded in OrderBook
```

Everything from step 3 to 5 is undone by one `catch (...)`: riders released, then stock. Allocation and pricing run *before* the first reservation, so a rejected coupon never touches inventory at all.

## Allocation: the 100 / 70 case

```
Cart: Apple ×100, Banana ×3, Chocolate ×2

                       Apple  Banana  Chocolate
Dhanmondi (1.4 km)      70      20      0
Gulshan   (2.2 km)       0       5      0
Mirpur    (3.0 km)      40       0     10

Pass 1 — is there one store that covers the whole cart?
  Dhanmondi: 70 < 100          ✗
  Gulshan:   0 < 100           ✗
  Mirpur:   40 < 100           ✗
  → split

Pass 2 — take what each store has, nearest first
  Dhanmondi: Apple 70, Banana 3          (Apple still outstanding: 30)
  Gulshan:   nothing left to take        (no shipment)
  Mirpur:    Apple 30, Chocolate 2       (fulfilled)

Result: 2 shipments, 2 riders, 2 × ৳30 delivery
```

Why pass 1 exists: an order served by one store means one rider, one delivery fee, and one bag. Split only when unavoidable.

The original snippet checked only the *first* store, and when that failed it fell into a split loop that never went back to the first store for the remaining units.

## Delivery

| Concept | Meaning |
|---|---|
| `ShipmentPlan` | What one store ships — the unit of delivery, not the whole order |
| `OrderShipment` | The recorded version: store, rider, distance, priced lines |
| `RiderPool` | Riders belonging to *one* store; dispatch happens at the source store |
| Capacity | A rider may carry several orders at once, so dispatch picks the **least-loaded rider with spare capacity** |

Each shipment carries its own store and its own rider. A shortage at any source store fails the order **before** stock is committed, and the message names the store.

All-or-nothing is deliberate: partial dispatch would need a partial-cancellation and partial-refund policy, which is a product decision, not an implementation detail.

## Money, payment, coupons

- **Money** is `int64` poisha. `double` money was one of the original's defects: repeated percentage discounts drift, and `₹` was hardcoded.
- **Payment is a Strategy.** `charge()` returns a result (`Captured`, or `Pending` for COD) or throws `CheckoutError`. A decline is a normal outcome, not a crash.
- **COD** is `Pending`: the order is recorded, the rider collects at the door, and `OrderBook` reports captured and pending revenue separately.
- **Coupon** is a single code: percentage off the item subtotal with a minimum spend. Delivery is never discounted. Unknown code or unmet minimum refuses the order before any stock is reserved.
- A **zero total** (100% coupon, free delivery) skips the rail entirely with reference `NO-CHARGE`.

## Schema and logic fixes

| Original | Problem | Now |
|---|---|---|
| `ProductFactory::createProduct` fabricated `"Item<sku>"` at ₹100 | a typo silently became a product | `ProductFactory::create` throws `unknown SKU` |
| `DbInventoryStore` owned product master *and* quantities | two stores could disagree about a price | `Catalog` (master) vs `Inventory` (quantities per store) |
| `map<int,int> stock` | no held state for an unpaid order | `onHand` / `reserved` / `available` with `reserve`/`commit`/`release` |
| `removeProduct` clamped silently, accepted negative qty | inventory drifted with no error | reservation guards; `stockIn` rejects `qty <= 0` |
| Stock removed *before* payment and rider checks | a declined payment destroyed inventory | reserve first, commit only after the charge succeeds |
| `Order.items` held `Product*` borrowed from the cart, then `delete`d them | double free / dangling pointers in stored orders | `OrderLine` owns a price snapshot |
| `items[]` and `partners[]` were parallel arrays | no relationship between a store's goods and its rider | `OrderShipment { store, rider, distance, lines }` |
| `allItems[sku] = qty` | a duplicate cart line overwrote the earlier quantity | cart merges lines per SKU |
| One `DeliveryPartner` even when only the single-store branch ran | rider count did not follow shipment count | one rider per shipment, at the source store |
| `double` money, hardcoded `₹` | rounding drift, no currency handling | `int64` poisha, `৳` with thousands grouping |
| `static int Order::nextId` | a failed checkout still consumed an id | `OrderBook` assigns ids, refusals consume none |
| `DarkStore::addStock` called a factory and dropped the pointer | leak per call | `inventory().stockIn(sku, qty)` |
| `new DarkStore` / `new InventoryManager` / `new Order` everywhere | leaks, exception-unsafe | values and `std::vector` |
| `5.0` radius inline in `placeOrder` | business rule hidden in a handler | `OrderAllocator(network, radius)` |
| No rider check at all | "successful" order with an unstaffed shipment | refused, with rollback |

## Edge cases handled

| Case | Behaviour |
|---|---|
| Empty cart / zero total | rejected |
| Quantity ≤ 0, negative price, unknown SKU | rejected at construction or lookup |
| Cart line added twice for the same SKU | merged; conflicting price rejected |
| `price × qty`, quantity sum, `taka → poisha` overflow | `overflow_error`, never a wrapped total |
| Drop point outside the radius | rejected, nothing reserved |
| Store exactly on the radius | included (`distance <= maxKm`) |
| One store covers everything | single shipment, single rider, single fee |
| No single store covers the cart | split nearest-first, `min(available, outstanding)` per store |
| Same line split across three stores | allowed; quantities still sum to the request |
| Total stock short of the request | refused, message names the missing `SKU xQty` |
| No rider / all riders at capacity | refused, stock released |
| Card declined / wallet balance too low | refused with the available amount, stock and riders released |
| Coupon below minimum / unknown code | refused before any reservation |
| Coupon that computes zero | not recorded as applied |
| Fully discounted order | accepted, payment skipped with `NO-CHARGE` |
| Cash on delivery | accepted, recorded unpaid, revenue reported as pending |
| Repeated checkout with the same cart | cart unchanged, re-allocates from *current* stock; the second fails honestly when stock is gone |
| Duplicate rider id / store name / SKU / coupon code | rejected on insert |
| Order id that skips the book sequence | `logic_error` |
| Discount greater than the item subtotal | `CheckoutSummary` refuses to exist |
| Order lines whose sum ≠ priced subtotal | `logic_error` before the order is recorded |

## Limitations

Deliberate scope cuts, not oversights:

- **Authorise → persist → capture.** The demo captures payment, then records the order. A crash in that window could take money without an order. Production would hold, persist, then capture, with reconciliation.
- **No concurrency control.** Allocation reads availability, then reserves. Another thread could take the units in between; the reserve throws and the checkout fails cleanly, but a real system needs an atomic conditional decrement.
- **Riders are never freed.** `commit` leaves them carrying the order forever; a real system decrements the load when the delivery completes.
- **All-or-nothing fulfilment**, because partial acceptance needs partial refunds.
- **One coupon, no stacking, no expiry.** See [`coupon-engine/`](../coupon-engine/) for the stacking model.
- **Planar geometry**, not haversine, and no road distance or ETA.
- **No persistence, tax, or time windows.** `OrderBook` is in memory.

## Verification

```bash
g++ -std=c++26 -Wall -Wextra -o main main.cpp && ./main
```

| Check | Result |
|---|---|
| Compiler warnings | none (`-Wall -Wextra`) |
| Demo self-checks | 25/25 passed, exit 0 |
| Adversarial harness (70 assertions: money, inventory, cart, rider dispatch, allocation, rollback, idempotency, model invariants) | 70/70 passed |
| Leak audit (`operator new`/`delete` counter), demo | `new=265 delete=265 leaked=0` |
| Leak audit, harness | `new=380 delete=380 leaked=0` |

## Demo walkthrough

1. **Rider shortage first.** The cart (Apple ×100, Banana ×3, Chocolate ×2) needs Dhanmondi + Mirpur, but no riders are hired. The order is refused *after* the reservations, and the demo re-reads availability to prove the 70 apples are still there.
2. **Retry with riders and coupon `EID10`.** Same cart object — checkout never mutated it. Two shipments, two riders, ৳2,130 items − ৳213 coupon + ৳60 delivery = ৳1,977 by bKash.
3. **Declined card.** A second cart needs Mirpur apples and Gulshan jeans; the card cannot cover ৳1,260. Stock, reservation and rider all return to their pre-checkout state.
4. **Refused carts that never touch stock.** Below-minimum coupon, unknown code, 500-apple shortage (`SKU 101 x490`), drop point outside the radius, empty cart.
5. **Cash on delivery.** Banana from Dhanmondi, chocolate from Mirpur, recorded unpaid; captured revenue stays ৳1,977 and pending revenue is ৳200.
6. **Final state.** Store stocks after two accepted orders, rider capacity, and the captured/pending split.

## Extending

- **Different allocation policy** — replace `OrderAllocator` (it is a plain class, not an interface). Add an interface only when a second policy actually exists.
- **New payment rail** — implement `PaymentMethod::charge`; checkout does not change.
- **New coupon rule** — replace `Coupon::discountFor`; checkout only calls it.
- **Delivery lifecycle** — add `OrderBook::markDelivered(id)` that calls `RiderPool::release`.
