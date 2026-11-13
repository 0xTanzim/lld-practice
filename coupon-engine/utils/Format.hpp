#ifndef COUPON_FORMAT_HPP
#define COUPON_FORMAT_HPP

#include <iomanip>
#include <sstream>
#include <string>

inline std::string percentText(int basisPoints) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << (basisPoints / 100.0);
    std::string text = out.str();
    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text;
}

#endif // COUPON_FORMAT_HPP
