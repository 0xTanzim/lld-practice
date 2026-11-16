#ifndef QC_LOCATION_HPP
#define QC_LOCATION_HPP

#include <cmath>

// Planar coordinates in kilometres; a real deployment would use latitude and
// longitude with a haversine distance instead.
struct Location {
    double x = 0.0;
    double y = 0.0;

    double distanceTo(const Location &other) const {
        return std::hypot(x - other.x, y - other.y);
    }
};

#endif // QC_LOCATION_HPP
