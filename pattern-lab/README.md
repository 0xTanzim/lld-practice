# Pattern Lab

One pattern = one file. Learn a pattern → code it here → run it. Grows with every pattern learned, grouped by GoF category.

**Total patterns learned: 12**

## Index

### Behavioral

| Pattern | File |
|---|---|
| Command | [`behavioral/command.cpp`](behavioral/command.cpp) |
| Template Method | [`behavioral/template_method.cpp`](behavioral/template_method.cpp) |
| Chain of Responsibility | [`behavioral/chain_of_responsibility.cpp`](behavioral/chain_of_responsibility.cpp) |
| Iterator | [`behavioral/iterator.cpp`](behavioral/iterator.cpp) |
| State | [`behavioral/state.cpp`](behavioral/state.cpp) |

### Creational

| Pattern | File |
|---|---|
| Factory Method | [`creational/factory_method.cpp`](creational/factory_method.cpp) |
| Builder — without it (the problem) | [`creational/builder/01_without_builder.cpp`](creational/builder/01_without_builder.cpp) |
| Builder — builder + director | [`creational/builder/02_builder_with_director.cpp`](creational/builder/02_builder_with_director.cpp) |
| Builder — step builder (order in the type) | [`creational/builder/03_step_builder.cpp`](creational/builder/03_step_builder.cpp) |
| Prototype | [`creational/prototype.cpp`](creational/prototype.cpp) |

### Structural

| Pattern | File |
|---|---|
| Bridge | [`structural/bridge.cpp`](structural/bridge.cpp) |
| Composite | [`structural/composite.cpp`](structural/composite.cpp) |
| Flyweight — without it (the problem) | [`structural/flyweight/01_without_flyweight.cpp`](structural/flyweight/01_without_flyweight.cpp) |
| Flyweight — with it (shared intrinsic state) | [`structural/flyweight/02_with_flyweight.cpp`](structural/flyweight/02_with_flyweight.cpp) |
| Proxy — Virtual (lazy load) | [`structural/proxy/virtual_proxy.cpp`](structural/proxy/virtual_proxy.cpp) |
| Proxy — Remote (network stand-in) | [`structural/proxy/remote_proxy.cpp`](structural/proxy/remote_proxy.cpp) |
| Proxy — Protection (access control) | [`structural/proxy/protection_proxy.cpp`](structural/proxy/protection_proxy.cpp) |

## Build & Run

```bash
# one pattern
g++ -std=c++26 -Wall -o structural/proxy structural/proxy.cpp && ./structural/proxy
```

Binaries stay untracked — `.gitignore` keeps only `.cpp` and `.md` inside `pattern-lab/`.

A pattern with several variants gets its own folder — see `structural/proxy/` (virtual, remote, protection) and `creational/builder/` (without, builder, step builder). The pattern is counted once.

## Adding a Pattern

1. Pick category folder: `behavioral/` | `creational/` | `structural/`
2. Create `<pattern>.cpp` — self-contained: interfaces + concrete classes + `main()` demo. More than one variant → `<pattern>/01_….cpp`, `<pattern>/02_….cpp`
3. Add row to that category's table, bump the total count

## Promotion Path

```
learn → pattern-lab/<category>/<pattern>.cpp   (single file, fast)
      → grows real? → top-level project folder  (tomato/, notification-system/)
```
