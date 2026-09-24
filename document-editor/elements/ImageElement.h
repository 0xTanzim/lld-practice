#ifndef IMAGE_ELEMENT_H
#define IMAGE_ELEMENT_H

#include "DocumentElement.h"

class ImageElement : public DocumentElement {
private:
    string imagePath;

public:
    explicit ImageElement(const string& imagePath) : imagePath(imagePath) {}

    string render() const override {
        return "[Image: " + imagePath + "]";
    }
};

#endif // IMAGE_ELEMENT_H
