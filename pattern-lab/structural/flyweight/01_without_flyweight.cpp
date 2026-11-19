#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// FLYWEIGHT, part 1 of 2 - the baseline, with no sharing at all.
//
// Every asteroid here carries its own copy of everything, including the six
// fields that depend only on its *type*. Three types are spawned a million
// times, so the same six values are stored a million times.
//
// 02_with_flyweight.cpp spawns the same asteroids with the intrinsic state
// pulled out and shared. The object construction in both files is the same
// Builder, so the only thing that changes between them is who owns the
// intrinsic state.
//
// Builder is doing real work here: ten values, split into two groups that
// positional arguments cannot tell apart. `withType(...)` and
// `withPosition(...)` name the groups; `Asteroid(25, 25, 250, "Red", ...)`
// does not.

struct AsteroidType {
    string name;
    int length;
    int width;
    int weight;
    string color;
    string texture;
    string material;
};

// The config the spawner reads. Note what this is *not*: it is not storage.
// Each asteroid copies these six values into itself.
const array<AsteroidType, 3> &asteroidTypes() {
    static const array<AsteroidType, 3> types{{
        {"rocky", 25, 25, 250, "Red", "Rocky", "Iron"},
        {"metallic", 35, 35, 350, "Blue", "Metallic", "Stone"},
        {"icy", 45, 45, 450, "Gray", "Icy", "Ice"},
    }};
    return types;
}

class Asteroid {
public:
    class Builder {
    public:
        Builder &withType(const AsteroidType &type) {
            length_ = type.length;
            width_ = type.width;
            weight_ = type.weight;
            color_ = type.color;
            texture_ = type.texture;
            material_ = type.material;
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

        Asteroid build() const {
            if (length_ <= 0) {
                throw logic_error("build: asteroid type is required");
            }
            return Asteroid(*this);
        }

    private:
        friend class Asteroid;

        int length_ = 0;
        int width_ = 0;
        int weight_ = 0;
        string color_;
        string texture_;
        string material_;
        int posX_ = 0;
        int posY_ = 0;
        int velocityX_ = 0;
        int velocityY_ = 0;
    };

    static Builder builder() {
        return Builder();
    }

    void render() const {
        cout << color_ << " " << texture_ << " / " << material_ << ", " << length_ << "x" << width_
             << ", " << weight_ << " kg at (" << posX_ << "," << posY_ << ") velocity ("
             << velocityX_ << "," << velocityY_ << ")\n";
    }

private:
    explicit Asteroid(const Builder &builder)
        : length_(builder.length_), width_(builder.width_), weight_(builder.weight_),
          color_(builder.color_), texture_(builder.texture_), material_(builder.material_),
          posX_(builder.posX_), posY_(builder.posY_), velocityX_(builder.velocityX_),
          velocityY_(builder.velocityY_) {}

    // Intrinsic: set once from the type, then never changed. Every asteroid
    // has its own copy, and every copy of a type is byte-identical.
    int length_;
    int width_;
    int weight_;
    string color_;
    string texture_;
    string material_;

    // Extrinsic: genuinely different per asteroid.
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
            asteroids_.push_back(Asteroid::builder()
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

    size_t memoryUsage() const {
        return asteroids_.size() * sizeof(Asteroid);
    }

private:
    vector<Asteroid> asteroids_;
};

int main() {
    // No magic number: one constant, used for the run and shown in the report.
    constexpr size_t kAsteroidCount = 1'000'000;

    cout << "Without Flyweight - every asteroid owns its own copy of its type\n";

    SpaceGame game;
    game.spawn(kAsteroidCount);
    game.renderFirst(5);

    const size_t total = game.memoryUsage();

    cout << "\n=== memory ===\n";
    cout << "  asteroids        : " << game.count() << "\n";
    cout << "  distinct types   : " << asteroidTypes().size() << "\n";
    cout << "  bytes per object : " << sizeof(Asteroid) << "\n";
    cout << "  total            : " << total << " bytes (";
    cout << fixed << setprecision(2) << total / (1024.0 * 1024.0) << " MB)\n";

    cout << "\nsizeof counts the three strings, and their inline buffer already holds\n"
            "values this short - so nothing here is allocated on the heap per\n"
            "asteroid. The cost is the copies themselves: six identical fields,\n"
            "stored once per object instead of once per type.\n";

    return 0;
}
