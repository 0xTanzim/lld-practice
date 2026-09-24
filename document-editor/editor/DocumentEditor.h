#ifndef DOCUMENT_EDITOR_H
#define DOCUMENT_EDITOR_H

#include <stdexcept>
#include <string>
#include "../elements/Document.h"
#include "../elements/TextElement.h"
#include "../elements/ImageElement.h"
#include "../elements/NewLineElement.h"
#include "../elements/TabSpaceElement.h"
#include "../persistence/Persistence.h"
using namespace std;

class DocumentEditor {
private:
    Document *document;
    Persistence *storage;

public:
    DocumentEditor(Document *document, Persistence *storage)
        : document(document), storage(storage) {
        if (!document || !storage) {
            throw invalid_argument("DocumentEditor: document and storage are required.");
        }
    }

    void addText(const string &text) {
        document->addElement(new TextElement(text));
    }

    void addImage(const string &imagePath) {
        document->addElement(new ImageElement(imagePath));
    }

    void addNewLine() {
        document->addElement(new NewLineElement());
    }

    void addTabSpace() {
        document->addElement(new TabSpaceElement());
    }

    string renderDocument() const {
        return document->render();
    }

    void saveDocument() {
        storage->save(renderDocument());
    }
};

#endif // DOCUMENT_EDITOR_H
