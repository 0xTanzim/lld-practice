#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// NOT THE PATTERN - this file is the problem the Builder exists to solve.
// It is kept next to the builder so the "why" is visible, not just the "how".
//
// Three defects, all of them shown in main():
//
//   1. Telescoping constructors - one overload per combination of fields, so
//      the argument list *is* the documentation and the position *is* the name.
//   2. A mutable product - setters let a request change after it was handed to
//      someone else, so "the request" is not one value, it is a moving target.
//   3. No required/optional distinction - the comment calls url, method and
//      timeout "required", and then the constructor invents defaults for two of
//      them. A forgotten field is silently replaced, never reported.

class HttpRequest {
public:
    HttpRequest(const string &url) : url_(url), method_("GET"), timeoutSeconds_(30) {}

    HttpRequest(const string &url, string method)
        : url_(url), method_(std::move(method)), timeoutSeconds_(30) {}

    HttpRequest(const string &url, string method, int timeoutSeconds)
        : url_(url), method_(std::move(method)), timeoutSeconds_(timeoutSeconds) {}

    HttpRequest(const string &url, string method, int timeoutSeconds,
                map<string, string> headers)
        : url_(url), method_(std::move(method)), timeoutSeconds_(timeoutSeconds),
          headers_(std::move(headers)) {}

    HttpRequest(const string &url, string method, int timeoutSeconds,
                map<string, string> headers, map<string, string> queryParams)
        : url_(url), method_(std::move(method)), timeoutSeconds_(timeoutSeconds),
          headers_(std::move(headers)), queryParams_(std::move(queryParams)) {}

    HttpRequest(const string &url, string method, int timeoutSeconds,
                map<string, string> headers, map<string, string> queryParams, string body)
        : url_(url), method_(std::move(method)), timeoutSeconds_(timeoutSeconds),
          headers_(std::move(headers)), queryParams_(std::move(queryParams)),
          body_(std::move(body)) {}

    // One more overload for every field added later. Six fields already need
    // six constructors; the count is not the problem, the ambiguity is.

    void setMethod(string method) {
        method_ = std::move(method);
    }

    void addHeader(const string &key, const string &value) {
        headers_[key] = value;
    }

    void addQueryParam(const string &key, const string &value) {
        queryParams_[key] = value;
    }

    void setBody(string body) {
        body_ = std::move(body);
    }

    void setTimeout(int seconds) {
        timeoutSeconds_ = seconds;
    }

    void execute() const {
        cout << "    " << method_ << " " << url_ << "\n";
        for (const auto &entry : queryParams_) {
            cout << "      ?" << entry.first << "=" << entry.second << "\n";
        }
        for (const auto &entry : headers_) {
            cout << "      " << entry.first << ": " << entry.second << "\n";
        }
        if (!body_.empty()) {
            cout << "      body: " << body_ << "\n";
        }
        cout << "      timeout: " << timeoutSeconds_ << "s\n";
    }

private:
    string url_;
    string method_;
    int timeoutSeconds_;
    map<string, string> headers_;
    map<string, string> queryParams_;
    string body_;
};

int main() {
    cout << "Without Builder - the problem\n\n";

    cout << "1. Telescoping constructors: the position is the field name\n";
    vector<HttpRequest> requests;
    requests.emplace_back("https://api.shop.dev/orders");
    requests.emplace_back("https://api.shop.dev/orders", "POST");
    requests.emplace_back("https://api.shop.dev/orders", "PUT", 60);
    for (const HttpRequest &request : requests) {
        request.execute();
    }
    cout << "  (url, method, 60) - 60 is a timeout today. Insert one field\n"
           "  before it and every existing call silently means something else.\n";

    cout << "\n2. A mutable product: the same object means two things\n";
    HttpRequest order("https://api.shop.dev/orders", "POST");
    auto send = [](const HttpRequest &request) { request.execute(); };

    send(order);
    order.setMethod("GET");
    send(order);
    cout << "  One object, sent twice, two different requests. Anything that\n"
           "  kept a reference now sees the second one instead of the first.\n";

    cout << "\n3. Forgetting a field is silent, because defaults exist\n";
    HttpRequest profile("https://api.shop.dev/profile");
    profile.execute();
    cout << "  The caller meant POST. It went GET. Nothing complained.\n";

    cout << "\n4. Nothing validates, because no single place owns the rules\n";
    order.setTimeout(0);
    order.setMethod("GETT");
    send(order);
    cout << "  timeout 0 and method \"GETT\" are accepted by the setters.\n";

    cout << "\nThe fix is not \"more constructors\". It is one place that\n"
            "collects the fields, one place that refuses an invalid\n"
            "combination, and a product that cannot change afterwards.\n";

    return 0;
}
