#include <sstream>
#include <string>

#include <cstddef>

#include <fmt/format.h>

#include "utils/parse.hpp"

ParseException::ParseException(std::string new_message) : message(std::move(new_message)) {}
const char* ParseException::what() const noexcept { return message.c_str(); }

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
