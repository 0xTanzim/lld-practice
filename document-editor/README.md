# Document Editor

LLD exercise: a document editor that composes elements (text, images, line breaks, tabs), renders them, and persists the output.

## Patterns Used

| Pattern | Where |
|---|---|
| Composite | `Document` holds `DocumentElement*` tree — leaf elements (`TextElement`, `ImageElement`, `NewLineElement`, `TabSpaceElement`) render uniformly |
| Strategy | `Persistence` — swap `FileStorage` / `DBStorage` at runtime |
| Facade | `DocumentEditor` — single entry point for client |

## Structure

```
document-editor/
├── main.cpp                  # Composition root
│
├── elements/                 # Composite layer
│   ├── DocumentElement.h     # Abstract element
│   ├── TextElement.h
│   ├── ImageElement.h
│   ├── NewLineElement.h
│   ├── TabSpaceElement.h
│   └── Document.h            # Composite container
│
├── persistence/              # Strategy layer
│   ├── Persistence.h         # Abstract strategy
│   ├── FileStorage.h
│   └── DBStorage.h
│
└── editor/
    └── DocumentEditor.h      # Facade
```

## Build & Run

```bash
g++ -std=c++17 -Wall -o main main.cpp
./main
```

Output is printed to stdout and saved to `document.txt`.

## Flow

```
addText/addImage/addNewLine/addTabSpace
    → renderDocument() → saveDocument()
```

## Extending

- **New element type**: subclass `DocumentElement`, implement `render()`, add `addXxx()` in `DocumentEditor`.
- **New storage target**: subclass `Persistence`, implement `save()`, inject into `DocumentEditor`.
