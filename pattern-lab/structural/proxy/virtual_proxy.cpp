#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
using namespace std;

class IImage {
public:
    virtual ~IImage() = default;
    virtual void display() const = 0;
};

class RealImage : public IImage {
private:
    string filename;

public:
    explicit RealImage(const string &file) : filename(file) {
        // Simulated expensive operation — the cost the proxy defers.
        cout << "[RealImage] Loading image from disk: " << filename << "\n";
    }

    void display() const override {
        cout << "[RealImage] Displaying " << filename << "\n";
    }
};

class ImageProxy : public IImage {
private:
    string filename;
    mutable unique_ptr<RealImage> realImage;

public:
    explicit ImageProxy(const string &file) : filename(file) {
        if (filename.empty()) {
            throw invalid_argument("Image filename must not be empty.");
        }
    }

    void display() const override {
        if (!realImage) {
            realImage = make_unique<RealImage>(filename);
        }
        realImage->display();
    }
};

int main() {
    ImageProxy image("sample.jpg");
    cout << "Proxy created — real image not loaded yet\n";

    image.display();
    image.display();

    return 0;
}
