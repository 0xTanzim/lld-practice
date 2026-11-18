#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

// BUILDER: assemble an object step by step, then hand it over finished.
//
// The product's constructor is private and the builder is its friend, so a
// partially configured request cannot exist: the only way in is build(), and
// build() is the last moment anything can be refused. Afterwards the request is
// a value - it can be copied, passed around and printed, but not modified.
//
// The DIRECTOR is optional. It owns a *recipe* (the shape a common request
// takes), not the parts: callers that need something custom skip it and drive
// the builder themselves.

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

    const map<string, string> &queryParams() const {
        return queryParams_;
    }

    const string &body() const {
        return body_;
    }

    int timeoutSeconds() const {
        return timeoutSeconds_;
    }

private:
    // Only the builder may construct one, and only once it is complete.
    friend class HttpRequestBuilder;

    HttpRequest(string url, string method, map<string, string> headers,
                map<string, string> queryParams, string body, int timeoutSeconds)
        : url_(std::move(url)), method_(std::move(method)), headers_(std::move(headers)),
          queryParams_(std::move(queryParams)), body_(std::move(body)),
          timeoutSeconds_(timeoutSeconds) {}

    string url_;
    string method_;
    map<string, string> headers_;
    map<string, string> queryParams_;
    string body_;
    int timeoutSeconds_;
};

class HttpRequestBuilder {
public:
    HttpRequestBuilder &withUrl(string url) {
        url_ = std::move(url);
        return *this;
    }

    HttpRequestBuilder &withMethod(string method) {
        method_ = std::move(method);
        return *this;
    }

    HttpRequestBuilder &withHeader(const string &key, const string &value) {
        headers_[key] = value;
        return *this;
    }

    HttpRequestBuilder &withQueryParam(const string &key, const string &value) {
        queryParams_[key] = value;
        return *this;
    }

    HttpRequestBuilder &withBody(string body) {
        body_ = std::move(body);
        return *this;
    }

    HttpRequestBuilder &withTimeout(int seconds) {
        timeoutSeconds_ = seconds;
        return *this;
    }

    HttpRequest build() const {
        if (url_.empty()) {
            throw invalid_argument("build: url is required");
        }
        if (url_.rfind("https://", 0) != 0) {
            throw invalid_argument("build: url must start with https://, got " + url_);
        }
        if (method_.empty()) {
            throw logic_error("build: method is required");
        }
        if (!isKnownMethod(method_)) {
            throw invalid_argument("build: unknown method " + method_);
        }
        if (timeoutSeconds_ <= 0) {
            throw invalid_argument("build: timeout must be positive");
        }
        if ((method_ == "GET" || method_ == "DELETE") && !body_.empty()) {
            throw logic_error("build: a " + method_ + " request cannot carry a body");
        }
        return HttpRequest(url_, method_, headers_, queryParams_, body_, timeoutSeconds_);
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
    map<string, string> queryParams_;
    string body_;
    // The one default that is a real policy rather than a hiding place: a
    // timeout is always needed and 30s is the product decision.
    int timeoutSeconds_ = 30;
};

// Optional part of the pattern: the recipes the app actually uses.
class HttpRequestDirector {
public:
    static HttpRequest get(const string &url) {
        return HttpRequestBuilder().withUrl(url).withMethod("GET").build();
    }

    static HttpRequest jsonPost(const string &url, string body) {
        return HttpRequestBuilder()
            .withUrl(url)
            .withMethod("POST")
            .withHeader("Content-Type", "application/json")
            .withHeader("Accept", "application/json")
            .withBody(std::move(body))
            .build();
    }

    static HttpRequest jsonPatch(const string &url, string body) {
        return HttpRequestBuilder()
            .withUrl(url)
            .withMethod("PATCH")
            .withHeader("Content-Type", "application/json")
            .withBody(std::move(body))
            .build();
    }
};

void execute(const HttpRequest &request) {
    cout << "    " << request.method() << " " << request.url() << "\n";
    for (const auto &entry : request.queryParams()) {
        cout << "      ?" << entry.first << "=" << entry.second << "\n";
    }
    for (const auto &entry : request.headers()) {
        cout << "      " << entry.first << ": " << entry.second << "\n";
    }
    if (!request.body().empty()) {
        cout << "      body: " << request.body() << "\n";
    }
    cout << "      timeout: " << request.timeoutSeconds() << "s\n";
}

int main() {
    cout << "Builder (+ optional Director)\n\n";

    cout << "1. Custom request: drive the builder directly\n";
    execute(HttpRequestBuilder()
                .withUrl("https://api.shop.dev/orders")
                .withMethod("POST")
                .withHeader("Content-Type", "application/json")
                .withHeader("Accept", "application/json")
                .withQueryParam("key", "12345")
                .withBody(R"({"sku":101,"qty":2})")
                .withTimeout(60)
                .build());

    cout << "\n2. Director, recipe: plain GET\n";
    execute(HttpRequestDirector::get("https://api.shop.dev/users"));

    cout << "\n3. Director, recipe: JSON POST\n";
    execute(HttpRequestDirector::jsonPost("https://api.shop.dev/users",
                                          R"({"name":"Aditya","email":"aditya@shop.dev"})"));

    cout << "\n4. The same builder instance builds two independent requests\n";
    HttpRequestBuilder builder;
    builder.withUrl("https://api.shop.dev/orders/7").withMethod("DELETE");
    const HttpRequest first = builder.build();
    builder.withUrl("https://api.shop.dev/orders/8").withMethod("DELETE");
    const HttpRequest second = builder.build();
    execute(first);
    execute(second);
    cout << "  build() copies out what it has; later changes do not reach\n"
           "  a request that was already built.\n";

    cout << "\n5. build() is where every rule is enforced\n";
    auto refuse = [](const string &label, auto action) {
        try {
            action();
            cout << "  " << label << ": accepted (should not happen)\n";
        } catch (const exception &error) {
            cout << "  " << label << ": " << error.what() << "\n";
        }
    };

    refuse("missing url", [] { HttpRequestBuilder().withMethod("GET").build(); });
    refuse("insecure url", [] {
        HttpRequestBuilder().withUrl("http://insecure.dev").withMethod("GET").build();
    });
    refuse("missing method", [] {
        HttpRequestBuilder().withUrl("https://api.shop.dev").build();
    });
    refuse("unusable method", [] {
        HttpRequestBuilder().withUrl("https://api.shop.dev").withMethod("GETT").build();
    });
    refuse("zero timeout", [] {
        HttpRequestBuilder().withUrl("https://api.shop.dev").withMethod("GET").withTimeout(0).build();
    });
    refuse("GET with a body", [] {
        HttpRequestBuilder()
            .withUrl("https://api.shop.dev")
            .withMethod("GET")
            .withBody("nope")
            .build();
    });

    cout << "\nNo HttpRequest exists in a partial state: the only way in is a\n"
            "complete build(), and after that it cannot be modified.\n";

    return 0;
}
