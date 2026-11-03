#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
using namespace std;

class IDataService {
public:
    virtual ~IDataService() = default;
    virtual string fetchData() = 0;
};

class RealDataService : public IDataService {
public:
    RealDataService() {
        // Simulated expensive remote connection.
        cout << "[RealDataService] Initialized (simulating remote setup)\n";
    }

    string fetchData() override {
        return "[RealDataService] Data from server";
    }
};

class DataServiceProxy : public IDataService {
private:
    string endpoint;
    unique_ptr<RealDataService> realService;

public:
    explicit DataServiceProxy(const string &endpoint) : endpoint(endpoint) {
        if (endpoint.empty()) {
            throw invalid_argument("Service endpoint must not be empty.");
        }
    }

    string fetchData() override {
        cout << "[DataServiceProxy] Connecting to " << endpoint << "...\n";
        if (!realService) {
            realService = make_unique<RealDataService>();
        }
        return realService->fetchData();
    }
};

int main() {
    DataServiceProxy dataService("https://api.example.com");
    cout << "Proxy created — no connection yet\n\n";

    cout << dataService.fetchData() << "\n";
    cout << dataService.fetchData() << "\n";

    return 0;
}
