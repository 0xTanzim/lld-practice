#ifndef DOCUMENT_ELEMENT_H
#define DOCUMENT_ELEMENT_H

#include <string>
using namespace std;

class DocumentElement {
public:
    virtual string render() const = 0;
    virtual ~DocumentElement() = default;
};

#endif // DOCUMENT_ELEMENT_H
