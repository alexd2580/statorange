#include <sstream>
#include <string>
#include <vector>

#include <cstddef>

#include <fmt/format.h>

#include "utils/parse.hpp"

bool is_unicode(std::string const& string) { return (string.at(0) & 0b10000000) != 0; }

std::string to_unicode(uint64_t codepoint) {
    std::vector<char> data;
    if(codepoint < 0x7F) {
        data.push_back(static_cast<char>(codepoint & 0xFFu));
    } else if(codepoint < 0x07FF) {
        data.push_back(static_cast<char>(0b11000000u | ((codepoint >> 6u) & 0b00011111u)));
        data.push_back(static_cast<char>(0b10000000u | (codepoint & 0b00111111u)));
    } else if(codepoint < 0xFFFF) {
        data.push_back(static_cast<char>(0b11100000u | ((codepoint >> 12u) & 0b00001111u)));
        data.push_back(static_cast<char>(0b10000000u | ((codepoint >> 6u) & 0b00111111u)));
        data.push_back(static_cast<char>(0b10000000u | (codepoint & 0b00111111u)));
    }
    data.push_back('\0');
    return std::string(data.data());
}

StringPointer::StringPointer(char const* new_base) : base(new_base), offset(0) {
    if(new_base == nullptr) {
        throw ParseException("Cannot create `StringPointer` to `nullptr`.");
    }
}

StringPointer::operator char const*() const { // NOLINT: Implicit conversion desired.
    return base + offset;                     // NOLINT: Pointer arithmetic intended.
}

char StringPointer::peek() const { return base[offset]; } // NOLINT: Pointer arithmetic intended.

void StringPointer::skip(size_t num) { offset += num; }

char StringPointer::next() {
    return base[offset++]; // NOLINT: Pointer arithmetic intended.
}

void StringPointer::whitespace() {
    char c = peek();
    while((c == ' ' || c == '\n' || c == '\t') && c != '\0') {
        c = base[++offset]; // NOLINT: Pointer arithmetic intended.
    }
}

void StringPointer::nonspace() {
    char c = peek();
    while(c != ' ' && c != '\n' && c != '\t' && c != '\0') {
        c = base[++offset]; // NOLINT: Pointer arithmetic intended.
    }
}

std::string StringPointer::escaped_string() {
    char c = peek();
    if(c != '"') {
        throw ParseException(fmt::format("Expected [\"], got [{}].", c));
    }
    skip(1);

    std::ostringstream o;
    ssize_t start = offset;
    c = next();

    while(c != '"' && c != '\0') {
        if(c == '\\') // If an escaped char is found.
        {
            // `str` points to one character ahead.
            // Push the existing string and grab the next character.
            ssize_t len = offset - start - 1;
            o << std::string(base + start, static_cast<size_t>(len)); // NOLINT: Pointer arithmetic intended.
            c = next();

            switch(c) {
            case '\0':
                throw ParseException("Unexpected EOS when parsing escaped character.");
            case 'n':
                o << '\n';
                break;
            case 't':
                o << '\t';
                break;
            case 'r':
                o << '\r';
                break;
            case '\\':
                o << '\\';
                break;
            case '"':
                o << '"';
                break;
            case 'u': {
                // TODO fix error reporting when stream ends. This part can cause a segmentation fault.
                char const c1 = next();
                char const c2 = next();
                char const c3 = next();
                char const c4 = next();
                std::string const as_hex = fmt::format("0x{}{}{}{}", c1, c2, c3, c4);
                uint64_t as_int = std::strtoul(as_hex.c_str(), nullptr, 16);
                o << to_unicode(as_int);
                break;
            }
            default:
                throw ParseException(fmt::format("Invalid escape sequence [\\{}].", c));
            }
            start = offset;
        }
        c = next();
    }

    if(c == '\0') {
        throw ParseException("Unexpected EOS.");
    }

    ssize_t len = offset - start - 1;
    o << std::string(base + start, static_cast<size_t>(len)); // NOLINT: Pointer arithmetic intended.

    return o.str();
}
