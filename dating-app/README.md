# Dating App — Matching, Notifications, Chat

LLD exercise: profiles, a swiping match engine, multi-channel notifications, and a chat room per match — behind one facade.

Starting point was a 971-line single-file snippet (`temp.tmp`). This version keeps the same domain, fixes the defects listed below, and uses only patterns already practiced elsewhere in this repo.

## Patterns Used

| Pattern | Where | What it does here |
|---|---|---|
| **Facade** | `app/DatingApp.hpp` | One object hides the user directory, matcher, chat rooms and notification hub. `swipe()` alone decides: record, detect the match, open the room, notify both people |
| **Observer** | `notifications/` | `NotificationService` is the subject; `EmailChannel` / `SmsChannel` / `PushChannel` are the observers |
| **Strategy** | `matching/rules/`, `matching/Matcher.hpp` | A score is assembled from interchangeable `ScoringRule`s (`PreferenceGate`, `SharedInterests`, `Proximity`) |
| **Factory** | `matching/MatcherFactory.hpp` | `MatcherType` → rule list. An unknown type throws instead of quietly downgrading |

That is the whole list. Buttons that exist only once are not patterns, they are just classes.

### What was deliberately left out

| Removed | Why |
|---|---|
| **Singleton** for the notification registry | It is the root cause of the bug below. A global registry forced `User`'s constructor and destructor to reach out and register/unregister — so improving `User` meant touching notifications. One `NotificationService` owned by the facade does the same job and can be destroyed. |
| The matcher inheritance chain | `LocationBasedMatcher` constructed an `InterestsBasedMatcher` which constructed a `BasicMatcher`. That is not a Strategy — it is three classes pretending to be one pipeline. Replaced by independent rules the matcher composes. |
| Separate `UserNotificationObserver` | It only printed. The three channels *are* the observers; a fourth class wrapping the same `cout` added nothing. |
| `LocationService` | It existed as a forward declaration and never got a definition. Distance lives on `Location`, where the two points are. |
| A `Match` object with its own id | A match *is* "a room exists between two people". `chatRoom(a, b)` answers it; a second source of truth would drift. |

## The notification bug

```cpp
// temp.tmp:45, :60
std::map<std::string, NotificationObserver*> observers;
...
observers[userId] = observer;      // assignment, not append
```

One observer per key. Register email, then push — push overwrites email. The user silently loses a channel, and nothing reports it.

The second half of the bug is *where* that registration lived:

```cpp
// temp.tmp:482-489
User::User(...)  { NotificationService::getInstance()->registerObserver(id, this); }
User::~User()    { NotificationService::getInstance()->removeObserver(id); }
```

Notifications were a side effect of `User`'s lifetime, reached through a global. That is exactly "improve `User`, lose a couple of notifications": any change to construction, copying or destruction changes the wiring, and the compiler cannot tell you.

Now:

```cpp
std::map<std::string, std::vector<std::unique_ptr<NotificationChannel>>> channels_;
```

- `subscribe()` **appends** — adding a channel is additive by construction.
- `unsubscribe()` removes one kind and leaves the rest registered.
- `notify()` delivers to every channel the user has and returns the count.
- `User` has **no** notification member at all.

Tomorrow's WhatsApp or Telegram channel is one class with three methods plus one `subscribe` call. No existing channel, no `User`, and no match code changes.

## Structure

```
dating-app/
├── main.cpp                          # Composition root + deterministic demo (41 self-checks)
│
├── models/                           # Value types; a half-built one cannot exist
│   ├── Gender.hpp
│   ├── Swipe.hpp                     # SwipeAction (left/right) + names
│   ├── Location.hpp                  # Haversine distance, validated latitude/longitude
│   ├── Interest.hpp                  # Name + category
│   ├── Preference.hpp                # Genders, age band, max distance
│   ├── Profile.hpp                   # Identity, bio, photos, interests
│   ├── User.hpp                      # Id + Profile + Preference + swipe history
│   ├── Message.hpp                   # Sender, text, timestamp
│   └── ChatRoom.hpp                  # Two participants + ordered messages
│
├── matching/
│   ├── RuleScore.hpp                 # accepted (hard gate) + points (bonus)
│   ├── ScoringRule.hpp               # One independent reason a pair fits
│   ├── Matcher.hpp                   # Runs the rules, combines, caps at 1.0
│   ├── MatcherFactory.hpp            # FACTORY: MatcherType -> rule list
│   └── rules/
│       ├── PreferenceGate.hpp        # Hard gate, worth 0.5 when passed
│       ├── SharedInterests.hpp       # Up to 0.3, scaled by overlap
│       └── Proximity.hpp             # Up to 0.2, scaled by closeness
│
├── notifications/                    # OBSERVER
│   ├── NotificationChannel.hpp       # Interface: name, destination, deliver
│   ├── EmailChannel.hpp
│   ├── SmsChannel.hpp
│   ├── PushChannel.hpp
│   └── NotificationService.hpp       # Subject: one user, many channels
│
├── app/
│   └── DatingApp.hpp                 # FACADE: users, candidates, swipe, chat, channels
│
├── errors/
│   └── AppError.hpp                  # One refusal type
│
└── utils/
    └── ConsoleStyle.hpp              # Colored, UTF-8-width-aware output
```

## Build & Run

```bash
g++ -std=c++26 -Wall -Wextra -o main main.cpp && ./main
```

No dependencies, no build system, single translation unit. The demo is deterministic: the two chat timestamps come from the clock, everything else is fixed.

## Flow

```
DatingApp::swipe(from, to, RIGHT)
    │
    ├─ 1. viewer.swipe(to, action)      # throws on self-swipe and repeat swipe
    ├─ 2. target.hasLiked(from)?        # no  -> done, one-sided like
    ├─ 3. already a room?               # yes -> logic_error, never a duplicate room
    ├─ 4. chatRooms_.emplace_back(...)  # the match is recorded as a room
    ├─ 5. notify(from, "It's a match…") # every channel the user registered
    └─ 6. notify(to,   "It's a match…")
```

`candidates(user, radius)` is the read side: skip yourself, skip anyone already swiped, skip anyone outside the radius, keep a score above zero, sort by score then id.

## Scoring: three rules, one number

```
score = 0.0                                  if any rule rejects
      = min(1.0, Σ accepted rule points)     otherwise
```

| Rule | Weight | Meaning |
|---|---|---|
| `PreferenceGate` | 0.5 | **Gate.** Both people must accept the other's gender, age and distance. 0.5 for passing — clearing each other's filters already makes a pair a maybe |
| `SharedInterests` | 0.3 × shared / longer list | Overlap, relative to the longer list |
| `Proximity` | 0.2 × (1 − distance / reach) | Full marks at 0 km, nothing at the shorter of the two reach limits |

```
Rohan → Neha     basic 0.50   interests 0.70   location 0.84    (3.1 km, 2 of 3 interests)
Rohan → Ayesha   basic 0.50   interests 0.60   location 0.66    (7.1 km, 1 of 3)
Rohan → Tanvir   basic 0.00   interests 0.00   location 0.00    (gate: Rohan wants women)
```

Why `Proximity` uses the *shorter* reach limit: a match both people can act on is limited by whoever is willing to travel less.

Every rule answers only for itself. Adding "distance from my work address" is one class with one `evaluate` plus one line in the factory — no existing rule, and no matcher, is touched.

## Schema and logic fixes

| Original | Problem | Now |
|---|---|---|
| `observers[userId] = observer` | a second channel replaced the first | `map<user, vector<unique_ptr<Channel>>>`, `subscribe` appends |
| `User` ctor/dtor registered with a global | notification wiring was a side effect of a model's lifetime | `DatingApp` owns channels; `User` has no notification member |
| `NotificationService::getInstance()` | global state, untestable, order-dependent | one instance owned by the facade |
| `ChatRoom` held `vector<Message*>` and deleted them | leaked on throw, double-freed on copy | `vector<Message>` by value |
| `Profile` held `vector<Interest*>` | same problem, plus shared ownership ambiguity | `vector<Interest>` by value |
| `swipeHistory[id] = action` | a second swipe on the same person flipped the decision | `User::swipe` refuses a repeat swipe |
| No self-swipe check | a user could match themselves | refused |
| Properties had default constructors | `Location(0,0)` is the Gulf of Guinea; `Preference(18,100,any)` matches everyone. Two unconfigured users "matched" | no default constructors; every field validated |
| Age unchecked | a 12-year-old could be a candidate | `Profile` and `Preference` require 18+ |
| `Location::distance` in a `LocationService` that was never written | accidental planar distance, or a compile error | Haversine on `Location`, `sqrt` argument clamped so antipodes are finite |
| Factory `default: return BasicMatcher()` | a typo silently downgraded matching | `throw` naming the type |
| `sendMessage` printed "no chat room" and continued | a failed send looked like a success | `throw AppError` |
| `sendMessage` never checked the receiver exists | a room lookup hid the real mistake | explicit existence check |
| `LocationBasedMatcher` built an `InterestsBasedMatcher` inside `calculateMatchScore` | "location" silently meant location + interests + basic; interests could not be used without basic; every new rule edited the class below | each rule is independent; the factory decides combinations |
| Interest matching by `Interest*` identity | two users with "Travel" did not match unless they shared the pointer | compared by name |
| `findUser` returned a null pointer, callers used it | null dereference | `user()` throws, `find()` returns `nullptr` and is the only nullable form |
| Dual `using namespace std;` plus a global map | header-level pollution, one registry for the process | nothing at namespace scope |

## Edge cases handled

| Case | Behaviour |
|---|---|
| Empty/blank name, id or bio? | name and id rejected when empty; bio may be empty |
| Age below 18 or above 120 | rejected at construction |
| Latitude outside −90..90, longitude outside −180..180, NaN | rejected |
| Antipodal points | finite distance (clamped), not `NaN` |
| Max distance ≤ 0 or beyond the earth's circumference | rejected |
| Max age below min age | rejected |
| Duplicate interest name or photo URL on one profile | rejected |
| Empty `interestedIn` list | interpreted as "open to everyone" |
| Non-mutual gender/age/distance preference | gate rejects in either direction |
| Two empty interest lists | 0 bonus, no division by zero |
| Distance exactly at a reach limit | accepted (`≤`) |
| Self-swipe | `AppError` |
| Second swipe on the same person | `AppError`, first decision preserved |
| A user with zero candidates | empty list, not an error |
| Radius ≤ 0 in `candidates` | rejected |
| Swipe on an unknown user | `AppError` |
| Match where a room already exists | `logic_error` — a duplicate room is a bug, not a feature |
| Message without a match | `AppError` |
| Message to an unknown receiver | `AppError` |
| Message from a non-participant directly on `ChatRoom` | `AppError` |
| Empty message text, empty sender | rejected |
| Duplicate user id | `AppError` |
| User with no channels | `notify` returns 0 — they simply hear nothing |
| Second channel of the same kind | rejected, so "email" is removable by name |
| Unsubscribe a kind that is not registered / unknown user | `AppError` |
| Unsubscribing the last channel | the subscriber disappears from the count |
| Unknown `MatcherType` | throws, never downgrades |

## Limitations

Deliberate scope cuts, not oversights:

- **No persistence.** Users, swipes, rooms and channels live in memory.
- **Swipes are one-shot.** No undo, no rewind, no "super like" budget.
- **No blocks, reports, or moderation.** A real product needs both sides of that.
- **No pagination.** `candidates()` scores every user; fine for a demo, not for a catalogue.
- **Distance is straight-line**, and there is no road distance, travel time, or country filter.
- **Chat has no read receipts, typing state, or delivery ordering beyond insertion order.** Timestamps come from the system clock.
- **`notify()` prints.** The channels are real observers with real validation, but delivery is `std::cout` rather than a transport.
- **The facade returns references** into its own storage, so a caller must not hold them across an `addUser`.

## Verification

```bash
g++ -std=c++26 -Wall -Wextra -o main main.cpp && ./main
```

| Check | Result |
|---|---|
| Compiler warnings | none (`-Wall -Wextra`) |
| Demo self-checks | 41/41 passed, exit 0 |
| Adversarial harness (109 assertions: location maths, model invariants, gate/rule arithmetic, matcher composition, channel fan-out, facade refusals) | 109/109 passed |
| Leak audit (`operator new`/`delete` counter), demo | `new=271 delete=271 leaked=0` |
| Leak audit, harness | `new=317 delete=317 leaked=0` |

Note on the harness: the first version of the "duplicate channel" case passed a *different* channel kind, so `subscribe` succeeded and its assertion block never ran. A `refuses(...)` helper silently skips its check when the action does not throw, so cases were moved to a second channel of the same kind and re-run.

## Demo walkthrough

1. **Four profiles.** Rohan, Neha, Tanvir, Ayesha with real coordinates around Dhaka; distances printed from Rohan (Neha 3.1 km, Tanvir 0.3 km, Ayesha 7.1 km).
2. **Same pair, three matchers.** The pipeline printed per matcher, so `basic` is visibly "the gate only", `interests` adds 0.3, `location` adds the last 0.2.
3. **One user, many channels.** Rohan subscribes email → one delivery, then push → two, then sms → three. Push is then removed and the other two keep working. This is the section that fails on `temp.tmp`.
4. **Candidates at 5 km.** Only Neha is in range and through the gate (0.84).
5. **Widen to 10 km.** Ayesha appears (0.66) and still sorts below Neha.
6. **Swipe twice.** Rohan → Neha is one-sided; Neha → Rohan creates the room and delivers to all three of Rohan's channels plus Neha's sms. Rohan's channel count is unchanged — activity on a `User` cannot disturb registration.
7. **Chat.** Two messages, in order, with sender names resolved through the facade.
8. **Refusals.** Self-swipe, repeat swipe, message without a match, unknown user, duplicate id, age 17, latitude 100, duplicate channel kind, unknown channel kind, unknown matcher type, zero radius — each prints the actual reason.
9. **Broadcast.** One `notifyAll` reaches all 5 channels; the count is asserted against `channelTotal()`.
10. **Final state.** Per-user swipes and channels, plus users/rooms/subscribers/channels and the active pipeline.

## Extending

- **New channel** — implement `NotificationChannel` (`name`, `destination`, `deliver`) and call `subscribe`. Nothing else changes; that is the point of the fix.
- **New scoring rule** — implement `ScoringRule::evaluate`, then add it to the rule list in `MatcherFactory::create`.
- **New matcher recipe** — one `MatcherType` enumerator and one `case`. The rules are already there, so "interests without proximity" or "gate plus age similarity" costs nothing.
- **Unmatch / block** — a facade method that removes the room and refuses future swipes in both directions; add `hasSwiped` cleanup plus a block set on `User`.
- **Persistence** — the models are values with validating constructors, so they map to rows without pointer fix-ups.
