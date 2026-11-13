#ifndef COUPON_CONSOLE_STYLE_HPP
#define COUPON_CONSOLE_STYLE_HPP

#include <iomanip>
#include <iostream>
#include <string>
using namespace std;

namespace Console {
constexpr const char *RESET = "\033[0m";
constexpr const char *BOLD = "\033[1m";
constexpr const char *DIM = "\033[2m";
constexpr const char *RED = "\033[31m";
constexpr const char *GREEN = "\033[32m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *CYAN = "\033[36m";

inline string repeat(const string &unit, size_t times) {
    string out;
    out.reserve(unit.size() * times);
    for (size_t i = 0; i < times; ++i) {
        out += unit;
    }
    return out;
}

// Terminal width is measured in code points so multi-byte symbols such as the
// taka sign still line up inside fixed-width columns.
inline size_t displayWidth(const string &text) {
    size_t width = 0;
    for (unsigned char byte : text) {
        if ((byte & 0xC0) != 0x80) {
            ++width;
        }
    }
    return width;
}

inline string padRight(const string &text, size_t width) {
    const size_t current = displayWidth(text);
    if (current >= width) {
        return text;
    }
    return text + string(width - current, ' ');
}

inline string padLeft(const string &text, size_t width) {
    const size_t current = displayWidth(text);
    if (current >= width) {
        return text;
    }
    return string(width - current, ' ') + text;
}

inline void banner(const string &title) {
    string inner = "  " + title + "  ";
    cout << "\n"
         << GREEN << "┌" << repeat("─", displayWidth(inner)) << "┐\n"
         << "│" << BOLD << inner << RESET << GREEN << "│\n"
         << "└" << repeat("─", displayWidth(inner)) << "┘" << RESET << "\n";
}

inline void section(const string &title) {
    cout << "\n"
         << BOLD << "▶ " << title << RESET << "\n"
         << DIM << repeat("─", displayWidth(title) + 3) << RESET << "\n";
}

inline void ok(const string &message) {
    cout << GREEN << "  ✓ " << message << RESET << "\n";
}

inline void fail(const string &message) {
    cout << RED << "  ✗ " << message << RESET << "\n";
}

inline void info(const string &message) {
    cout << CYAN << "  ○ " << message << RESET << "\n";
}

inline void warn(const string &message) {
    cout << YELLOW << "  ! " << message << RESET << "\n";
}

inline void kv(const string &key, const string &value) {
    cout << "    " << DIM << left << setw(12) << key << RESET << ": " << value << "\n";
}

inline void divider() {
    cout << DIM << repeat("─", 56) << RESET << "\n";
}
}

#endif // COUPON_CONSOLE_STYLE_HPP
