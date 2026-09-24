#ifndef TAB_SPACE_ELEMENT_H
#define TAB_SPACE_ELEMENT_H

#include "DocumentElement.h"

class TabSpaceElement : public DocumentElement {
public:
    string render() const override {
        return "\t";
    }
};

#endif // TAB_SPACE_ELEMENT_H
