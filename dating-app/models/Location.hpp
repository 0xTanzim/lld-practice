#ifndef DATING_LOCATION_HPP
#define DATING_LOCATION_HPP

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>

// A point on the earth. There is no default constructor on purpose: the
// original defaulted to (0, 0) - the Gulf of Guinea - so two users who had never
// set a location were "0 km apart" and matched each other.
class Location {
public:
    Location(double latitude, double longitude) : latitude_(latitude), longitude_(longitude) {
        if (!std::isfinite(latitude_) || latitude_ < -90.0 || latitude_ > 90.0) {
            throw std::invalid_argument("Location: latitude must be between -90 and 90");
        }
        if (!std::isfinite(longitude_) || longitude_ < -180.0 || longitude_ > 180.0) {
            throw std::invalid_argument("Location: longitude must be between -180 and 180");
        }
    }

    double latitude() const {
        return latitude_;
    }

    double longitude() const {
        return longitude_;
    }

    // Great-circle distance, Haversine. The angular term is clamped before the
    // square root: floating point can push `a` just past 1.0 for antipodal
    // points, and sqrt(1 - a) would then be NaN.
    double distanceKm(const Location &other) const {
        const double radians = std::numbers::pi / 180.0;
        const double deltaLat = (other.latitude_ - latitude_) * radians;
        const double deltaLon = (other.longitude_ - longitude_) * radians;

        const double sinLat = std::sin(deltaLat / 2.0);
        const double sinLon = std::sin(deltaLon / 2.0);
        double a = sinLat * sinLat
                   + std::cos(latitude_ * radians) * std::cos(other.latitude_ * radians) * sinLon
                         * sinLon;
        a = std::clamp(a, 0.0, 1.0);

        const double angular = 2.0 * std::asin(std::sqrt(a));
        return EARTH_RADIUS_KM * angular;
    }

    std::string format() const {
        std::ostringstream out;
        out << std::fixed << std::setprecision(4) << latitude_ << ", " << longitude_;
        return out.str();
    }

private:
    static constexpr double EARTH_RADIUS_KM = 6371.0;

    double latitude_;
    double longitude_;
};

#endif // DATING_LOCATION_HPP
