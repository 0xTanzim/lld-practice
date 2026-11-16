#include <iostream>
using namespace std;

// Implementor hierarchy: engines vary independently of car bodies.
class Engine {
public:
    virtual ~Engine() = default;
    virtual void start() const = 0;
};

class PetrolEngine : public Engine {
public:
    void start() const override {
        cout << "  Petrol engine: spark ignition\n";
    }
};

class DieselEngine : public Engine {
public:
    void start() const override {
        cout << "  Diesel engine: compression ignition\n";
    }
};

class ElectricEngine : public Engine {
public:
    void start() const override {
        cout << "  Electric engine: powering up silently\n";
    }
};

// Abstraction hierarchy: holds a non-owning engine, so every body can be
// paired with every engine and the pairing can change at runtime.
class Car {
public:
    explicit Car(const Engine &engine) : engine_(&engine) {}

    virtual ~Car() = default;

    void setEngine(const Engine &engine) {
        engine_ = &engine;
    }

    virtual void drive() const = 0;

protected:
    const Engine &engine() const {
        return *engine_;
    }

private:
    const Engine *engine_;
};

class Sedan : public Car {
public:
    using Car::Car;

    void drive() const override {
        cout << "Sedan on the highway\n";
        engine().start();
    }
};

class SUV : public Car {
public:
    using Car::Car;

    void drive() const override {
        cout << "SUV on the trail\n";
        engine().start();
    }
};

class Coupe : public Car {
public:
    using Car::Car;

    void drive() const override {
        cout << "Coupe on the coastal road\n";
        engine().start();
    }
};

int main() {
    PetrolEngine petrol;
    DieselEngine diesel;
    ElectricEngine electric;

    cout << "Bridge: 3 bodies x 3 engines, composed instead of subclassed\n\n";
    Sedan sedan(petrol);
    SUV suv(electric);
    Coupe coupe(diesel);
    sedan.drive();
    suv.drive();
    coupe.drive();

    cout << "\nSwap the implementor without touching the body:\n";
    sedan.setEngine(electric);
    sedan.drive();
    suv.setEngine(petrol);
    suv.drive();

    cout << "\nSame engine, different abstraction:\n";
    sedan.setEngine(diesel);
    coupe.setEngine(diesel);
    sedan.drive();
    coupe.drive();

    return 0;
}
