#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// VISITOR - a new operation over a closed set of types, without editing them.
//
// Double dispatch is the whole trick: accept() knows the concrete element,
// visit() knows the concrete visitor, and the two calls together pick the body.
// The element never names an operation, and no code here casts.
//
// The version this came from called accept(new SizeCalculationVisitor()) once
// per file and never deleted any of them, and every one of its nine visit()
// methods printed the same sentence with a different noun. Two things are
// fixed:
//
//   - visitors are stack objects passed by reference, so the caller owns them
//     and nothing can leak;
//   - the visitors below compute genuinely different things per type - one
//     sizes each file with its own formula, one previews it in its own shape -
//     which is the reason the pattern exists at all.
//
// The trade is stated at the end: a new operation is one class, a new type is
// one visit() in every visitor.

namespace {

constexpr size_t bytesPerKb = 1000;
constexpr size_t bytesPerMb = bytesPerKb * bytesPerKb;
constexpr size_t previewChars = 24;

string countOf(size_t count, const string &noun) {
    return to_string(count) + " " + noun + (count == 1 ? "" : "s");
}

string scaled(size_t bytes, double unit, const string &suffix) {
    ostringstream out;
    out << fixed << setprecision(1) << static_cast<double>(bytes) / unit << " " << suffix;
    return out.str();
}

// One decimal is enough for a console report, and it keeps the column narrow.
string humanBytes(size_t bytes) {
    if (bytes < bytesPerKb) {
        return to_string(bytes) + " B";
    }
    if (bytes < bytesPerMb) {
        return scaled(bytes, static_cast<double>(bytesPerKb), "kB");
    }
    return scaled(bytes, static_cast<double>(bytesPerMb), "MB");
}

string firstLine(const string &content) {
    const size_t end = content.find('\n');
    string line = content.substr(0, end);
    if (line.empty()) {
        return "(empty)";
    }
    if (line.size() > previewChars) {
        line = line.substr(0, previewChars) + "...";
    }
    return "\"" + line + "\"";
}

int gcd(int left, int right) {
    while (right != 0) {
        const int remainder = left % right;
        left = right;
        right = remainder;
    }
    return left;
}

}

class TextFile;
class ImageFile;
class VideoFile;

class FileVisitor {
public:
    virtual ~FileVisitor() = default;

    virtual void visit(const TextFile &file) = 0;
    virtual void visit(const ImageFile &file) = 0;
    virtual void visit(const VideoFile &file) = 0;
};

class FileSystemItem {
public:
    explicit FileSystemItem(string name) : name_(std::move(name)) {
        if (name_.empty()) {
            throw invalid_argument("FileSystemItem: name must not be empty");
        }
    }

    virtual ~FileSystemItem() = default;

    const string &name() const {
        return name_;
    }

    // The element decides which visit() overload runs, so the caller only ever
    // holds a FileSystemItem and still reaches type-specific behaviour.
    virtual void accept(FileVisitor &visitor) const = 0;

private:
    string name_;
};

class TextFile : public FileSystemItem {
public:
    TextFile(string name, string content)
        : FileSystemItem(std::move(name)), content_(std::move(content)) {}

    const string &content() const {
        return content_;
    }

    void accept(FileVisitor &visitor) const override {
        visitor.visit(*this);
    }

private:
    string content_;
};

class ImageFile : public FileSystemItem {
public:
    // Red, green, blue, no alpha: the stored size is one pixel per three bytes.
    static constexpr size_t bytesPerPixel = 3;

    ImageFile(string name, int width, int height)
        : FileSystemItem(std::move(name)), width_(width), height_(height) {
        if (width_ <= 0 || height_ <= 0) {
            throw invalid_argument("ImageFile: width and height must be positive");
        }
    }

    int width() const {
        return width_;
    }

    int height() const {
        return height_;
    }

    void accept(FileVisitor &visitor) const override {
        visitor.visit(*this);
    }

private:
    int width_;
    int height_;
};

class VideoFile : public FileSystemItem {
public:
    VideoFile(string name, int minutes, int megabytesPerMinute)
        : FileSystemItem(std::move(name)),
          minutes_(minutes),
          megabytesPerMinute_(megabytesPerMinute) {
        if (minutes_ <= 0) {
            throw invalid_argument("VideoFile: duration must be positive");
        }
        if (megabytesPerMinute_ <= 0) {
            throw invalid_argument("VideoFile: bitrate must be positive");
        }
    }

    int minutes() const {
        return minutes_;
    }

    int megabytesPerMinute() const {
        return megabytesPerMinute_;
    }

    void accept(FileVisitor &visitor) const override {
        visitor.visit(*this);
    }

private:
    int minutes_;
    int megabytesPerMinute_;
};

struct FileSize {
    string name;
    string kind;
    size_t bytes;
};

// Operation 1: how much space the folder takes. Each type answers with a
// different formula, which is the reason this cannot live in one base method.
class SizeReportVisitor : public FileVisitor {
public:
    void visit(const TextFile &file) override {
        record(file, "text", file.content().size());
    }

    void visit(const ImageFile &file) override {
        record(file, "image", static_cast<size_t>(file.width()) * file.height() *
                                  ImageFile::bytesPerPixel);
    }

    void visit(const VideoFile &file) override {
        record(file, "video", static_cast<size_t>(file.minutes()) * file.megabytesPerMinute() *
                                  bytesPerMb);
    }

    const vector<FileSize> &rows() const {
        return rows_;
    }

    size_t totalBytes() const {
        return total_;
    }

private:
    void record(const FileSystemItem &file, string kind, size_t bytes) {
        rows_.push_back({file.name(), std::move(kind), bytes});
        total_ += bytes;
    }

    vector<FileSize> rows_;
    size_t total_ = 0;
};

// Operation 2: one readable line per file. Same elements, no change to any of
// the three classes above.
class PreviewVisitor : public FileVisitor {
public:
    void visit(const TextFile &file) override {
        lines_.push_back(file.name() + ": " + firstLine(file.content()));
    }

    void visit(const ImageFile &file) override {
        const int divisor = gcd(file.width(), file.height());
        lines_.push_back(file.name() + ": " + to_string(file.width()) + "x" +
                         to_string(file.height()) + " (" + to_string(file.width() / divisor) + ":" +
                         to_string(file.height() / divisor) + ")");
    }

    void visit(const VideoFile &file) override {
        lines_.push_back(file.name() + ": " + countOf(static_cast<size_t>(file.minutes()), "minute") +
                         " at " + to_string(file.megabytesPerMinute()) + " MB/min");
    }

    const vector<string> &lines() const {
        return lines_;
    }

private:
    vector<string> lines_;
};

int main() {
    cout << "Visitor - new operations over a fixed set of types\n";

    vector<unique_ptr<FileSystemItem>> folder;
    folder.push_back(make_unique<TextFile>("notes.txt", "groceries: milk, eggs, coffee"));
    folder.push_back(make_unique<ImageFile>("diagram.png", 3200, 2400));
    folder.push_back(make_unique<VideoFile>("lecture.mp4", 45, 16));
    folder.push_back(make_unique<TextFile>("budget.txt", "rent 1200\nfood 400"));

    cout << "\n1. The folder\n   ";
    for (size_t index = 0; index < folder.size(); ++index) {
        cout << (index == 0 ? "" : ", ") << folder[index]->name();
    }
    cout << "\n   " << countOf(folder.size(), "file") << "\n";

    cout << "\n2. Operation 1 - size, computed differently per type\n";
    SizeReportVisitor sizes;
    for (const unique_ptr<FileSystemItem> &item : folder) {
        item->accept(sizes);
    }
    cout << "   " << left << setw(14) << "file" << setw(7) << "kind" << "size\n";
    for (const FileSize &row : sizes.rows()) {
        cout << "   " << left << setw(14) << row.name << setw(7) << row.kind << humanBytes(row.bytes)
             << "\n";
    }
    cout << "   total " << humanBytes(sizes.totalBytes()) << " in "
         << countOf(folder.size(), "file") << "\n";

    cout << "\n3. Operation 2 - preview, per type as well\n";
    PreviewVisitor previews;
    for (const unique_ptr<FileSystemItem> &item : folder) {
        item->accept(previews);
    }
    for (const string &line : previews.lines()) {
        cout << "   " << line << "\n";
    }

    cout << "\nThe three file classes were written once and did not change between the\n"
            "two operations: that is what the pattern buys.\n"
            "\n"
            "What it costs is honest too. A fourth file type means a new visit()\n"
            "overload in every visitor - the compiler enforces that, because they are\n"
            "pure virtual. The alternative, a dynamic_cast chain inside a free\n"
            "function, compiles happily after a new type is added and silently skips\n"
            "it. The pattern turns a silent miss into a build error.\n";

    return 0;
}
