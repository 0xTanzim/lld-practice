#ifndef TEXT_ELEMENT_H
#define TEXT_ELEMENT_H

#include "DocumentElement.h"

class TextElement : public DocumentElement {
private:
    string text;

public:
    explicit TextElement(const string& text) : text(text) {}

    string render() const override {
        return text;
    }
};

#endif // TEXT_ELEMENT_H
