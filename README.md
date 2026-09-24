# LLD — Low-Level Design

Practice repository for low-level design (LLD) exercises — SOLID principles, design patterns, and class-level architecture.

Each project is a self-contained design problem with its own README, build instructions, and structure.

---

## Projects

| # | Project | Problem | Language | Status |
|---|---------|---------|----------|--------|
| 1 | [`tomato/`](tomato/) | Online food ordering system | C++17 | ✅ Done |
| 2 | [`document-editor/`](document-editor/) | Composite document renderer with pluggable persistence | C++17 | ✅ Done |
| — | _project-name/_ | _one-line problem statement_ | _C++17_ | 🔜 Planned |

---

## Adding a New Project

1. Create folder: `lld/<project-name>/`
2. Add `<project-name>/README.md` — problem statement, patterns used, build & run
3. Add one row to the [Projects](#projects) table
4. Follow repo [Conventions](#conventions) below

Done. Root README never needs tomato-specific or project-specific details — those live in each project's own README.

---

## Repository Layout

```
lld/
├── README.md            # This file — generic project index
├── .gitignore           # Shared across all projects
│
├── <project-a>/         # Self-contained: own README + build
├── <project-b>/
└── ...
```

---

## Conventions

**Structure**
- Every project self-contained: own `README.md`, own build instructions
- Root README = index only, project README = details
- Group code by layer/feature (`models/`, `services/`, `strategies/`, `factories/`, `utils/`, ...)
- Shared ignore rules live only in root `.gitignore`

**Code**
- No verbose WHAT-comments — code reads as documentation
- No hardcoded magic values — externalize to constants/config
- Fail fast, explicit error handling, no silent failures
- SOLID, composition over inheritance, small focused units
- Group by feature/domain — no flat dumps

---

## Quick Start

```bash
git clone <repo-url> && cd lld
# Pick a project, then follow its README:
ls
```
