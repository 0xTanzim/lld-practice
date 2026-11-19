#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// FLYWEIGHT, part 2 of 2 - the intrinsic state is shared.
//
// The rule the pattern is built on: split an object's state in two.
//
//   intrinsic - same for every object of a type (size, weight, colour,
//               texture, material). It belongs to the *type*, so one instance
//               of it can serve millions of objects.
//   extrinsic - different per object (position, velocity). It belongs to the
//               object, so each object keeps its own.
//
// Nothing here is new storage: AsteroidContext is a pointer plus the four
// extrinsic ints, and the pointer is the same value for every asteroid of the
// same type. 01_without_flyweight.cpp stores all ten fields in every object.
//
// Builder is still doing the construction - and it hides the split. The caller
// writes `withType(...)`, never `getOrCreate(...)`; the cache lookup happens
// inside build(). Two patterns, one call site.

struct AsteroidType {
    string name;
    int length;
    int width;
    int weight;
    string color;
    string texture;
    string material;
};

const array<AsteroidType, 3> &asteroidTypes() {
    static const array<AsteroidType, 3> types{{
        {"rocky", 25, 25, 250, "Red", "Rocky", "Iron"},
        {"metallic", 35, 35, 350, "Blue", "Metallic", "Stone"},
        {"icy", 45, 45, 450, "Gray", "Icy", "Ice"},
    }};
    return types;
}

// The intrinsic half. Constructed once per type, never modified afterwards,
// and never owned by an asteroid.
class AsteroidFlyweight {
public:
    void render(int posX, int posY, int velocityX, int velocityY) const {
        cout << color_ << " " << texture_ << " / " << material_ << ", " << length_ << "x" << width_
             << ", " << weight_ << " kg at (" << posX << "," << posY << ") velocity ("
             << velocityX << "," << velocityY << ")\n";
    }

private:
    // Only the factory may create one - that is what keeps them shared.
    friend class AsteroidFactory;

    explicit AsteroidFlyweight(const AsteroidType &type)
        : length_(type.length), width_(type.width), weight_(type.weight), color_(type.color),
          texture_(type.texture), material_(type.material) {}

    int length_;
    int width_;
    int weight_;
    string color_;
    string texture_;
    string material_;
};

class AsteroidFactory {
public:
    // One flyweight per type, however many asteroids ask for it.
    const AsteroidFlyweight &getOrCreate(const AsteroidType &type) {
        unique_ptr<AsteroidFlyweight> &slot = flyweights_[type.name];
        if (!slot) {
            // Not make_unique: the constructor is private, and only a friend
            // may call it.
            slot = unique_ptr<AsteroidFlyweight>(new AsteroidFlyweight(type));
        }
        return *slot;
    }

    size_t flyweightCount() const {
        return flyweights_.size();
    }

    size_t memoryUsage() const {
        // The payload only. The map's own nodes and the key strings are extra,
        // and are counted in neither file.
        return flyweights_.size() * sizeof(AsteroidFlyweight);
    }

private:
    unordered_map<string, unique_ptr<AsteroidFlyweight>> flyweights_;
};

// The extrinsic half, plus a borrowed pointer to the shared half.
class AsteroidContext {
public:
    class Builder {
    public:
        explicit Builder(AsteroidFactory &factory) : factory_(factory) {}

        Builder &withType(const AsteroidType &type) {
            type_ = &type;
            return *this;
        }

        Builder &withPosition(int x, int y) {
            posX_ = x;
            posY_ = y;
            return *this;
        }

        Builder &withVelocity(int x, int y) {
            velocityX_ = x;
            velocityY_ = y;
            return *this;
        }

        AsteroidContext build() const {
            if (type_ == nullptr) {
                throw logic_error("build: asteroid type is required");
            }
            return AsteroidContext(factory_.getOrCreate(*type_), posX_, posY_, velocityX_,
                                   velocityY_);
        }

    private:
        friend class AsteroidContext;

        AsteroidFactory &factory_;
        const AsteroidType *type_ = nullptr;
        int posX_ = 0;
        int posY_ = 0;
        int velocityX_ = 0;
        int velocityY_ = 0;
    };

    static Builder builder(AsteroidFactory &factory) {
        return Builder(factory);
    }

    void render() const {
        flyweight_->render(posX_, posY_, velocityX_, velocityY_);
    }

private:
    AsteroidContext(const AsteroidFlyweight &flyweight, int posX, int posY, int velocityX,
                    int velocityY)
        : flyweight_(&flyweight), posX_(posX), posY_(posY), velocityX_(velocityX),
          velocityY_(velocityY) {}

    // Borrowed, not owned: the factory holds the only owner and outlives every
    // context built from it.
    const AsteroidFlyweight *flyweight_;

    int posX_;
    int posY_;
    int velocityX_;
    int velocityY_;
};

class SpaceGame {
public:
    void spawn(size_t count) {
        asteroids_.clear();
        asteroids_.reserve(count);

        const auto &types = asteroidTypes();
        for (size_t index = 0; index < count; ++index) {
            const int step = static_cast<int>(index);
            asteroids_.push_back(AsteroidContext::builder(factory_)
                                     .withType(types[index % types.size()])
                                     .withPosition(100 + step * 50, 200 + step * 30)
                                     .withVelocity(1, 2)
                                     .build());
        }
    }

    void renderFirst(size_t limit) const {
        const size_t shown = min(limit, asteroids_.size());
        cout << "\n--- first " << shown << " of " << asteroids_.size() << " ---\n";
        for (size_t index = 0; index < shown; ++index) {
            cout << "  #" << (index + 1) << " ";
            asteroids_[index].render();
        }
    }

    size_t count() const {
        return asteroids_.size();
    }

    size_t flyweightCount() const {
        return factory_.flyweightCount();
    }

    size_t memoryUsage() const {
        return asteroids_.size() * sizeof(AsteroidContext) + factory_.memoryUsage();
    }

private:
    AsteroidFactory factory_;
    vector<AsteroidContext> asteroids_;
};

int main() {
    constexpr size_t kAsteroidCount = 1'000'000;

    cout << "With Flyweight - the type is stored once and shared\n";

    SpaceGame game;
    game.spawn(kAsteroidCount);
    game.renderFirst(5);

    const size_t total = game.memoryUsage();

    cout << "\n=== memory ===\n";
    cout << "  asteroids        : " << game.count() << "\n";
    cout << "  flyweights       : " << game.flyweightCount() << "\n";
    cout << "  bytes per context: " << sizeof(AsteroidContext) << "\n";
    cout << "  bytes per type   : " << sizeof(AsteroidFlyweight) << "\n";
    cout << "  total            : " << total << " bytes (";
    cout << fixed << setprecision(2) << total / (1024.0 * 1024.0) << " MB)\n";

    cout << "\n" << game.count() << " asteroids share " << game.flyweightCount()
         << " flyweights. A context is a pointer plus the four extrinsic ints\n"
            "and nothing else; the six intrinsic fields live in one flyweight\n"
            "that every asteroid of that type points at.\n"
            "\n"
            "The saving here is only a few times, because the intrinsic state is\n"
            "three short strings. Put a texture buffer in the flyweight instead\n"
            "and the ratio becomes the size of that buffer - which is the case\n"
            "the pattern actually exists for.\n";

    return 0;
}
