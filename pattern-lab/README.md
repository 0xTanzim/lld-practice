# Pattern Lab

One pattern = one file. Learn a pattern → code it here → run it. Grows with every pattern learned, grouped by GoF category.

**Total patterns learned: 5**

## Index

### Behavioral

| Pattern | File |
|---|---|
| Command | [`behavioral/command.cpp`](behavioral/command.cpp) |
| Template Method | [`behavioral/template_method.cpp`](behavioral/template_method.cpp) |
| Chain of Responsibility | [`behavioral/chain_of_responsibility.cpp`](behavioral/chain_of_responsibility.cpp) |

### Creational

| Pattern | File |
|---|---|
| — | _none yet_ |

### Structural

| Pattern | File |
|---|---|
| Composite | [`structural/composite.cpp`](structural/composite.cpp) |
| Proxy — Virtual (lazy load) | [`structural/proxy/virtual_proxy.cpp`](structural/proxy/virtual_proxy.cpp) |
| Proxy — Remote (network stand-in) | [`structural/proxy/remote_proxy.cpp`](structural/proxy/remote_proxy.cpp) |
| Proxy — Protection (access control) | [`structural/proxy/protection_proxy.cpp`](structural/proxy/protection_proxy.cpp) |

## Build & Run

```bash
# one pattern
g++ -std=c++26 -Wall -o structural/proxy structural/proxy.cpp && ./structural/proxy
```

Binaries stay untracked — `.gitignore` keeps only `.cpp` and `.md` inside `pattern-lab/`.

## Adding a Pattern

1. Pick category folder: `behavioral/` | `creational/` | `structural/`
2. Create `<pattern>.cpp` — self-contained: interfaces + concrete classes + `main()` demo
3. Add row to that category's table, bump the total count

## Promotion Path

```
learn → pattern-lab/<category>/<pattern>.cpp   (single file, fast)
      → grows real? → top-level project folder  (tomato/, notification-system/)
```
