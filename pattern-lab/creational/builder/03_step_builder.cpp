#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

// STEP BUILDER: the same Builder, with the order enforced by the type system.
//
// A plain fluent builder lets you write .withBody(...) before .withUrl(...), or
// forget the method entirely, and you only find out when build() throws at
// runtime. A step builder hands back a *different type* after each call, so an
// out-of-order or incomplete chain does not compile at all.
//
//     UrlStep -> MethodStep -> HeaderStep -> OptionalStep -> HttpRequest
//     required   required       required      any of these
//
// Typical chain:
//
//     HttpRequest order = HttpRequestSteps().start()
//         .withUrl("https://api.shop.dev/products")
//         .withMethod("POST")
//         .withHeader("Content-Type", "application/json")
//         .withBody(R"({"sku":101})")
//         .build();
//
// Cost: four small interfaces plus multiple inheritance. Worth it when a wrong
// order is a real defect, not when the chain is three calls long.

class HttpRequest {
public:
    const string &url() const {
        return url_;
    }

    const string &method() const {
        return method_;
    }

    const map<string, string> &headers() const {
        return headers_;
    }

    const string &body() const {
        return body_;
    }

    int timeoutSeconds() const {
        return timeoutSeconds_;
    }

private:
    friend class HttpRequestStepBuilder;

    HttpRequest(string url, string method, map<string, string> headers, string body,
                int timeoutSeconds)
        : url_(std::move(url)), method_(std::move(method)), headers_(std::move(headers)),
          body_(std::move(body)), timeoutSeconds_(timeoutSeconds) {}

    string url_;
    string method_;
    map<string, string> headers_;
    string body_;
    int timeoutSeconds_;
};

class MethodStep;
class HeaderStep;
class OptionalStep;

// No virtual destructors: these interfaces exist to constrain the *return type*
// of the next call. Nothing is ever deleted through them - the concrete builder
// is a value that lives until the end of the chain expression.
class UrlStep {
public:
    virtual MethodStep &withUrl(string url) = 0;
};

class MethodStep {
public:
    virtual HeaderStep &withMethod(string method) = 0;
};

class HeaderStep {
public:
    virtual OptionalStep &withHeader(const string &key, const string &value) = 0;
};

class OptionalStep {
public:
    // Repeatable: the first header is required to get here, and every header
    // after it stays in this same step.
    virtual OptionalStep &withHeader(const string &key, const string &value) = 0;
    virtual OptionalStep &withBody(string body) = 0;
    virtual OptionalStep &withTimeout(int seconds) = 0;
    virtual HttpRequest build() const = 0;
};

class HttpRequestStepBuilder : public UrlStep,
                               public MethodStep,
                               public HeaderStep,
                               public OptionalStep {
public:
    MethodStep &withUrl(string url) override {
        url_ = std::move(url);
        return *this;
    }

    HeaderStep &withMethod(string method) override {
        method_ = std::move(method);
        return *this;
    }

    OptionalStep &withHeader(const string &key, const string &value) override {
        headers_[key] = value;
        return *this;
    }

    OptionalStep &withBody(string body) override {
        body_ = std::move(body);
        return *this;
    }

    OptionalStep &withTimeout(int seconds) override {
        timeoutSeconds_ = seconds;
        return *this;
    }

    HttpRequest build() const override {
        if (url_.empty()) {
            throw invalid_argument("build: url must not be empty");
        }
        if (url_.rfind("https://", 0) != 0) {
            throw invalid_argument("build: url must start with https://, got " + url_);
        }
        if (!isKnownMethod(method_)) {
            throw invalid_argument("build: unknown method " + method_);
        }
        if (timeoutSeconds_ <= 0) {
            throw invalid_argument("build: timeout must be positive");
        }
        return HttpRequest(url_, method_, headers_, body_, timeoutSeconds_);
    }

private:
    static bool isKnownMethod(const string &method) {
        for (const char *known : {"GET", "POST", "PUT", "PATCH", "DELETE"}) {
            if (method == known) {
                return true;
            }
        }
        return false;
    }

    string url_;
    string method_;
    map<string, string> headers_;
    string body_;
    int timeoutSeconds_ = 30;
};

// The entry point. It exists because start() cannot be a static member
// returning the concrete builder by value - that would put every step method
// back on the first line and the ordering guarantee would be gone - and it
// cannot be `return *new HttpRequestStepBuilder();` either, which leaks one
// builder per request.
//
// A holder owns the builder and exposes exactly one reference, typed UrlStep.
// The holder is a temporary, so the whole chain has to be a single expression:
// the builder dies at the closing semicolon, at the same moment the request it
// produced becomes a value.
class HttpRequestSteps {
public:
    UrlStep &start() {
        return builder_;
    }

private:
    HttpRequestStepBuilder builder_;
};

void execute(const HttpRequest &request) {
    cout << "    " << request.method() << " " << request.url() << "\n";
    for (const auto &entry : request.headers()) {
        cout << "      " << entry.first << ": " << entry.second << "\n";
    }
    if (!request.body().empty()) {
        cout << "      body: " << request.body() << "\n";
    }
    cout << "      timeout: " << request.timeoutSeconds() << "s\n";
}

int main() {
    cout << "Step Builder\n\n";

    cout << "1. The chain enforces url -> method -> header -> rest\n";
    const HttpRequest order =
        HttpRequestSteps().start()
            .withUrl("https://api.shop.dev/products")
            .withMethod("POST")
            .withHeader("Content-Type", "application/json")
            .withBody(R"({"product":"Laptop","price":49999})")
            .withTimeout(45)
            .build();
    execute(order);

    cout << "\n2. Two headers are fine: withHeader returns OptionalStep again\n";
    const HttpRequest user = HttpRequestSteps().start()
                                 .withUrl("https://api.shop.dev/users")
                                 .withMethod("DELETE")
                                 .withHeader("Accept", "application/json")
                                 .withHeader("Authorization", "Bearer <token>")
                                 .build();
    execute(user);

    cout << "\n3. What the types refuse to compile\n"
            "     HttpRequestSteps().start().withBody(\"{}\")\n"
            "       ^ no withUrl: UrlStep offers withUrl and nothing else\n"
            "     HttpRequestSteps().start().withUrl(u).withHeader(\"a\", \"b\")\n"
            "       ^ no withMethod: MethodStep offers withMethod and nothing else\n"
            "     HttpRequestSteps().start().withUrl(u).withMethod(\"GET\").build()\n"
            "       ^ no header: build() is out of reach until HeaderStep\n";

    cout << "\n4. Types constrain order, values still need checking\n";
    auto refuse = [](const string &label, auto action) {
        try {
            action();
            cout << "  " << label << ": accepted (should not happen)\n";
        } catch (const exception &error) {
            cout << "  " << label << ": " << error.what() << "\n";
        }
    };

    refuse("empty url", [] {
        HttpRequestSteps().start().withUrl("").withMethod("GET").withHeader("Accept", "*/*").build();
    });
    refuse("insecure url", [] {
        HttpRequestSteps().start()
            .withUrl("ftp://files.shop.dev")
            .withMethod("GET")
            .withHeader("Accept", "*/*")
            .build();
    });
    refuse("unusable method", [] {
        HttpRequestSteps().start()
            .withUrl("https://api.shop.dev")
            .withMethod("FETCH")
            .withHeader("Accept", "*/*")
            .build();
    });
    refuse("zero timeout", [] {
        HttpRequestSteps().start()
            .withUrl("https://api.shop.dev")
            .withMethod("GET")
            .withHeader("Accept", "*/*")
            .withTimeout(0)
            .build();
    });

    cout << "\nThe compiler proved the order; build() proved the values.\n";

    return 0;
}
