#include <iostream>
#include "editor/DocumentEditor.h"
#include "elements/Document.h"
#include "persistence/FileStorage.h"
using namespace std;

int main() {
    Document *document = new Document();
    Persistence *storage = new FileStorage();
    DocumentEditor *editor = new DocumentEditor(document, storage);

    editor->addText("Hello, world!");
    editor->addNewLine();
    editor->addText("This is a real-world document editor example.");
    editor->addNewLine();
    editor->addTabSpace();
    editor->addText("Indented text after a tab space.");
    editor->addNewLine();
    editor->addImage("picture.jpg");

    cout << editor->renderDocument() << endl;

    editor->saveDocument();

    delete editor;
    delete storage;
    delete document;

    return 0;
}
