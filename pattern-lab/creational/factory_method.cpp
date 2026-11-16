#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;

// FACTORY METHOD vs the simple factories used elsewhere in this repo.
//
// The other projects build products with a switch inside one function:
//
//     static PaymentMethod* make(GatewayType type) { switch (type) { ... } }
//
// That is a Simple Factory - the CLIENT names the concrete class, and every new
// product edits the same switch.
//
// Factory Method moves the decision into the class itself: the creator owns the
// algorithm and the product's lifetime, and its subclasses answer one question -
// "what concrete class do I stand for?" Adding a format means adding one class
// and editing nothing that already compiles.

class Document {
public:
    virtual ~Document() = default;
    virtual string mimeType() const = 0;
    virtual void render() const = 0;
};

class PdfDocument : public Document {
public:
    string mimeType() const override {
        return "application/pdf";
    }

    void render() const override {
        cout << "      %PDF-1.7 · pages laid out, fonts embedded\n";
    }
};

class HtmlDocument : public Document {
public:
    string mimeType() const override {
        return "text/html";
    }

    void render() const override {
        cout << "      <article><h1>Report</h1><p>...</p></article>\n";
    }
};

class MarkdownDocument : public Document {
public:
    string mimeType() const override {
        return "text/markdown";
    }

    void render() const override {
        cout << "      # Report\n"
                "      \n"
                "      - point one\n"
                "      - point two\n";
    }
};

// The creator. It owns the whole export pipeline and the product's lifetime.
// The one thing it cannot know is which concrete document a format produces -
// that single blank is the factory method.
class Exporter {
public:
    virtual ~Exporter() = default;

    void exportReport(const string &title) const {
        const unique_ptr<Document> document = createDocument();
        cout << "  " << title << " → " << document->mimeType() << "\n";
        document->render();
    }

protected:
    // The hook. Subclasses fill it in; the base class calls it.
    virtual unique_ptr<Document> createDocument() const = 0;
};

class PdfExporter : public Exporter {
protected:
    unique_ptr<Document> createDocument() const override {
        return make_unique<PdfDocument>();
    }
};

class HtmlExporter : public Exporter {
protected:
    unique_ptr<Document> createDocument() const override {
        return make_unique<HtmlDocument>();
    }
};

class MarkdownExporter : public Exporter {
protected:
    unique_ptr<Document> createDocument() const override {
        return make_unique<MarkdownDocument>();
    }
};

int main() {
    cout << "Factory Method\n\n";

    // The caller holds Exporter, never Document, and never writes a type switch.
    vector<unique_ptr<Exporter>> exporters;
    exporters.push_back(make_unique<PdfExporter>());
    exporters.push_back(make_unique<HtmlExporter>());
    exporters.push_back(make_unique<MarkdownExporter>());

    for (const unique_ptr<Exporter> &exporter : exporters) {
        exporter->exportReport("Q3 report");
        cout << "\n";
    }

    cout << "A fourth format is one more Exporter subclass. exportReport() and\n"
            "every existing exporter stay untouched - there is no switch to grow.\n";

    return 0;
}
