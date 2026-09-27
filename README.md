# lld-practice

Hands-on **Low-Level Design** practice in C++26 — one design problem at a time. Modular code, design patterns, SOLID principles.

> **New project** → new folder + own `README.md` + one row in [Projects](#projects).
> **Learning a pattern** → drop one file in [`pattern-lab/`](pattern-lab/README.md).

## Projects

| # | Project | Problem | Patterns | Status |
|---|---------|---------|----------|--------|
| 1 | [`tomato/`](tomato/) | Online food ordering system | Strategy, Abstract Factory, Singleton, Facade | ✅ Done |
| 2 | [`document-editor/`](document-editor/) | Document rendering + persistence | Composite, Strategy, Facade | ✅ Done |
| 3 | [`notification-system/`](notification-system/) | Notification pipeline | Decorator, Observer, Strategy, Singleton | ✅ Done |

## Pattern Lab

Single-pattern scratchpad, grouped by GoF category — grows with every pattern learned.

| Category | Learned |
|---|---|
| Behavioral | [Command](pattern-lab/behavioral/command.cpp) |
| Creational | — |
| Structural | — |

Full index: [`pattern-lab/README.md`](pattern-lab/README.md)

## Layout

```
lld-practice/
├── tomato/                # Full project — own README + build
├── document-editor/
├── notification-system/
├── pattern-lab/           # One-pattern experiments by GoF category
│   ├── behavioral/
│   ├── creational/
│   └── structural/
├── README.md              # Index — you are here
└── .gitignore             # Shared rules
```

Each project is self-contained: own `README.md`, own build instructions. Details live there — this file stays an index.

## Conventions

- **C++26** — `g++ -std=c++26 -Wall`
- No verbose WHAT-comments — code reads as documentation
- No magic values — constants centralized
- Fail fast, explicit errors, no silent failures
- SOLID, composition over inheritance
- Code grouped by layer/feature (`models/`, `strategies/`, `observers/`, ...)

## Getting Started

```bash
git clone https://github.com/<your-username>/lld-practice.git
cd lld-practice
```

Then follow any project's README, e.g.:

```bash
cd notification-system
g++ -std=c++26 -Wall -o main main.cpp && ./main
```
