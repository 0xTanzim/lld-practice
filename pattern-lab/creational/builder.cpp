#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

// BUILDER: assemble an object step by step, then hand it over finished.
//
// The point is not "many constructor arguments". The point is that the product
// is immutable: every field is set once, by the builder, inside the product's
// own constructor. There is no moment where a half-configured request exists,
// and no setter that can change a request already in flight.
//
// build() is also the last place that can refuse - a GET with a body, an
// http:// URL, a timeout of zero. Those rules live in one function instead of
// being re-checked by every caller.

class HttpRequest {
public:
    const string &method() const {
        return method_;
    }

    const string &url() const {
        return url_;
    }

    const map<string, string> &headers() const {
        return headers_;
    }

    const string &body() const {
        return body_;
    }

    int timeoutMs() const {
        return timeoutMs_;
    }

private:
    // Only the builder may construct one, and only once it is complete.
    friend class HttpRequestBuilder;

    HttpRequest(string method, string url, map<string, string> headers, string body, int timeoutMs)
        : method_(std::move(method)), url_(std::move(url)), headers_(std::move(headers)),
          body_(std::move(body)), timeoutMs_(timeoutMs) {}

    string method_;
    string url_;
    map<string, string> headers_;
    string body_;
    int timeoutMs_;
};

class HttpRequestBuilder {
public:
    HttpRequestBuilder &method(string value) {
        method_ = std::move(value);
        return *this;
    }

    HttpRequestBuilder &url(string value) {
        url_ = std::move(value);
        return *this;
    }

    HttpRequestBuilder &header(string name, string value) {
        headers_[std::move(name)] = std::move(value);
        return *this;
    }

    HttpRequestBuilder &body(string value) {
        body_ = std::move(value);
        return *this;
    }

    HttpRequestBuilder &timeout(int milliseconds) {
        timeoutMs_ = milliseconds;
        return *this;
    }

    HttpRequest build() const {
        if (method_.empty()) {
            throw logic_error("build: method is required");
        }
        if (url_.rfind("https://", 0) != 0) {
            throw invalid_argument("build: url must start with https://, got " + url_);
        }
        if (method_ == "GET" && !body_.empty()) {
            throw logic_error("build: a GET request cannot carry a body");
        }
        if (timeoutMs_ <= 0) {
            throw invalid_argument("build: timeout must be positive");
        }
        return HttpRequest(method_, url_, headers_, body_, timeoutMs_);
    }

private:
    string method_;
    string url_;
    map<string, string> headers_;
    string body_;
    int timeoutMs_ = 5000;
};

// Optional part of the pattern. The director owns the *recipe*, not the parts:
// it drives the same builder through a known shape. Callers that need a custom
// request skip the director and drive the builder themselves.
class RequestDirector {
public:
    static HttpRequest jsonGet(HttpRequestBuilder builder, const string &url) {
        return builder.method("GET").url(url).header("Accept", "application/json").build();
    }

    static HttpRequest jsonPost(HttpRequestBuilder builder, const string &url, string body) {
        return builder.method("POST")
            .url(url)
            .header("Accept", "application/json")
            .header("Content-Type", "application/json")
            .body(std::move(body))
            .build();
    }
};

void printRequest(const HttpRequest &request) {
    cout << "  " << request.method() << " " << request.url()
         << "  (timeout " << request.timeoutMs() << " ms)\n";
    for (const auto &entry : request.headers()) {
        cout << "    " << entry.first << ": " << entry.second << "\n";
    }
    if (!request.body().empty()) {
        cout << "    body: " << request.body() << "\n";
    }
}

int main() {
    cout << "Builder\n\n";

    cout << "1. Director, preset recipe\n";
    printRequest(RequestDirector::jsonGet(HttpRequestBuilder(), "https://api.shop.dev/orders"));

    cout << "\n2. Director, POST with body\n";
    printRequest(RequestDirector::jsonPost(HttpRequestBuilder(), "https://api.shop.dev/orders",
                                           R"({"sku":101,"qty":2})"));

    cout << "\n3. No director: drive the builder directly\n";
    printRequest(HttpRequestBuilder()
                     .method("PUT")
                     .url("https://api.shop.dev/orders/7")
                     .header("Content-Type", "application/json")
                     .header("Authorization", "Bearer <token>")
                     .body(R"({"status":"shipped"})")
                     .timeout(1500)
                     .build());

    cout << "\n4. build() refuses an incomplete or contradictory request\n";
    try {
        HttpRequestBuilder().method("GET").url("http://insecure.dev").build();
    } catch (const invalid_argument &error) {
        cout << "  refused: " << error.what() << "\n";
    }
    try {
        HttpRequestBuilder().method("GET").url("https://api.shop.dev").body("nope").build();
    } catch (const logic_error &error) {
        cout << "  refused: " << error.what() << "\n";
    }
    try {
        HttpRequestBuilder().url("https://api.shop.dev").build();
    } catch (const logic_error &error) {
        cout << "  refused: " << error.what() << "\n";
    }

    cout << "\nNo HttpRequest exists in a partial state: the only way in is a\n"
            "complete build(), and after that it cannot be modified.\n";

    return 0;
}
