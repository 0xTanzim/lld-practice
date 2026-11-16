#ifndef QC_CONSOLE_STYLE_HPP
#define QC_CONSOLE_STYLE_HPP

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>

namespace Console {
constexpr const char *RESET = "\033[0m";
constexpr const char *BOLD = "\033[1m";
constexpr const char *DIM = "\033[2m";
constexpr const char *RED = "\033[31m";
constexpr const char *GREEN = "\033[32m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *BLUE = "\033[34m";
constexpr const char *CYAN = "\033[36m";

inline std::string repeat(const std::string &unit, std::size_t times) {
    std::string out;
    out.reserve(unit.size() * times);
    for (std::size_t index = 0; index < times; ++index) {
        out += unit;
    }
    return out;
}

// Width is counted in code points, not bytes: the taka sign and the box-drawing
// characters are multi-byte, so string::size() would misalign every column.
inline std::size_t displayWidth(const std::string &text) {
    std::size_t width = 0;
    for (unsigned char byte : text) {
        if ((byte & 0xC0) != 0x80) {
            ++width;
        }
    }
    return width;
}

inline std::string padRight(const std::string &text, std::size_t width) {
    const std::size_t current = displayWidth(text);
    return current >= width ? text : text + std::string(width - current, ' ');
}

inline std::string padLeft(const std::string &text, std::size_t width) {
    const std::size_t current = displayWidth(text);
    return current >= width ? text : std::string(width - current, ' ') + text;
}

inline void banner(const std::string &title) {
    const std::string inner = "  " + title + "  ";
    const std::size_t width = displayWidth(inner);
    std::cout << "\n"
              << GREEN << "┌" << repeat("─", width) << "┐\n"
              << "│" << BOLD << inner << RESET << GREEN << "│\n"
              << "└" << repeat("─", width) << "┘" << RESET << "\n";
}

inline void section(const std::string &title) {
    std::cout << "\n" << BOLD << "▶ " << title << RESET << "\n"
              << DIM << repeat("─", displayWidth(title) + 3) << RESET << "\n";
}

inline void ok(const std::string &message) {
    std::cout << GREEN << "  ✓ " << message << RESET << "\n";
}

inline void fail(const std::string &message) {
    std::cout << RED << "  ✗ " << message << RESET << "\n";
}

inline void info(const std::string &message) {
    std::cout << CYAN << "  ○ " << message << RESET << "\n";
}

inline void warn(const std::string &message) {
    std::cout << YELLOW << "  ! " << message << RESET << "\n";
}

inline void kv(const std::string &key, const std::string &value) {
    std::cout << "    " << DIM << std::left << std::setw(16) << key << RESET << ": " << value
              << "\n";
}

inline void divider(std::size_t width = 66) {
    std::cout << DIM << repeat("─", width) << RESET << "\n";
}
}

#endif // QC_CONSOLE_STYLE_HPP
