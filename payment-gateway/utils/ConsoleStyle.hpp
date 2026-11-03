#ifndef CONSOLE_STYLE_HPP
#define CONSOLE_STYLE_HPP

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

inline void banner(const string &title) {
    string inner = "  " + title + "  ";
    cout << "\n"
         << GREEN << "┌" << repeat("─", inner.size()) << "┐\n"
         << "│" << BOLD << inner << RESET << GREEN << "│\n"
         << "└" << repeat("─", inner.size()) << "┘" << RESET << "\n";
}

inline void section(const string &title) {
    cout << "\n"
         << BOLD << "▶ " << title << RESET << "\n"
         << DIM << repeat("─", title.size() + 3) << RESET << "\n";
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

#endif // CONSOLE_STYLE_HPP
