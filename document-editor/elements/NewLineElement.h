#ifndef NEW_LINE_ELEMENT_H
#define NEW_LINE_ELEMENT_H

#include "DocumentElement.h"

class NewLineElement : public DocumentElement {
public:
    string render() const override {
        return "\n";
    }
};

#endif // NEW_LINE_ELEMENT_H
