#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
using namespace std;

class FileSystemItem {
public:
    virtual ~FileSystemItem() = default;
    virtual void ls(int indent = 0) const = 0;
    virtual void openAll(int indent = 0) const = 0;
    virtual int getSize() const = 0;
    virtual const FileSystemItem* cd(const string &name) const = 0;
    virtual const string &getName() const = 0;
    virtual bool isFolder() const = 0;
};

class File : public FileSystemItem {
private:
    string name;
    int size;

public:
    File(const string &n, int s) : name(n), size(s) {}

    void ls(int indent = 0) const override {
        cout << string(indent, ' ') << name << "\n";
    }

    void openAll(int indent = 0) const override {
        cout << string(indent, ' ') << name << "\n";
    }

    int getSize() const override {
        return size;
    }

    const FileSystemItem *cd(const string &) const override {
        return nullptr;
    }

    const string &getName() const override {
        return name;
    }

    bool isFolder() const override {
        return false;
    }
};

class Folder : public FileSystemItem {
private:
    string name;
    vector<FileSystemItem *> children;

public:
    explicit Folder(const string &n) : name(n) {}

    ~Folder() override {
        for (auto child : children) {
            delete child;
        }
    }

    void add(FileSystemItem *item) {
        if (!item) {
            throw invalid_argument("Cannot add null item to folder.");
        }
        children.push_back(item);
    }

    void ls(int indent = 0) const override {
        for (auto child : children) {
            string prefix = child->isFolder() ? "+ " : "";
            cout << string(indent, ' ') << prefix << child->getName() << "\n";
        }
    }

    void openAll(int indent = 0) const override {
        cout << string(indent, ' ') << "+ " << name << "\n";
        for (auto child : children) {
            child->openAll(indent + 4);
        }
    }

    int getSize() const override {
        int total = 0;
        for (auto child : children) {
            total += child->getSize();
        }
        return total;
    }

    const FileSystemItem *cd(const string &target) const override {
        for (auto child : children) {
            if (child->isFolder() && child->getName() == target) {
                return child;
            }
        }
        return nullptr;
    }

    const string &getName() const override {
        return name;
    }

    bool isFolder() const override {
        return true;
    }
};

int main() {
    auto *root = new Folder("root");
    root->add(new File("file1.txt", 1));
    root->add(new File("file2.txt", 1));

    auto *docs = new Folder("docs");
    docs->add(new File("resume.pdf", 1));
    docs->add(new File("notes.txt", 1));
    root->add(docs);

    auto *images = new Folder("images");
    images->add(new File("photo.jpg", 1));
    root->add(images);

    root->ls();
    cout << "---\n";
    docs->ls();
    cout << "---\n";
    root->openAll();
    cout << "---\n";

    const FileSystemItem *cwd = root->cd("docs");
    if (cwd != nullptr) {
        cwd->ls();
    } else {
        cout << "Could not cd into docs\n";
    }

    cout << "Total size: " << root->getSize() << "\n";

    delete root;
    return 0;
}
