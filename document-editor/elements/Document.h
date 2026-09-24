#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <string>
#include <vector>
#include "DocumentElement.h"
using namespace std;

class Document {
private:
    vector<DocumentElement *> elements;

public:
    ~Document() {
        for (auto element : elements) {
            delete element;
        }
    }

    void addElement(DocumentElement *element) {
        elements.push_back(element);
    }

    string render() const {
        string result;
        for (auto element : elements) {
            result += element->render();
        }
        return result;
    }
};

#endif // DOCUMENT_H
