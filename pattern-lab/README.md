# Pattern Lab

One pattern = one file. Learn a pattern → code it here → run it. Grows with every pattern learned, grouped by GoF category.

**Total patterns learned: 1**

## Index

### Behavioral

| Pattern | File |
|---|---|
| Command | [`behavioral/command.cpp`](behavioral/command.cpp) |

### Creational

| Pattern | File |
|---|---|
| — | _none yet_ |

### Structural

| Pattern | File |
|---|---|
| — | _none yet_ |

## Build & Run

```bash
g++ -std=c++26 -Wall -o behavioral/command behavioral/command.cpp && ./behavioral/command
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
