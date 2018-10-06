#ifndef UTILS_PARSE_HPP
#define UTILS_PARSE_HPP

#include <exception>
#include <string>

#include <cstddef>

#include "utils/convert.hpp"

class ParseException : public std::exception {
  private:
    std::string message;

  public:
    explicit ParseException(std::string message);
    const char* what() const noexcept override;
};

class StringPointer final {
  private:
    char const* const base;
    ssize_t offset;

  public:
    explicit StringPointer(char const* base);
    operator char const*() const; // NOLINT: Implicit conversion desired.

    char peek() const;
    void skip(size_t offset);
    char next();
    void whitespace();
    void nonspace();
    // NOLINTNEXTLINE: Desired call signature.
    std::string escaped_string();

    template <typename T>
    T number() {
        bool valid;
        char* endptr;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        auto n = convert_to_number<T>(base + offset, valid, endptr);
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        if(!valid || endptr == base + offset) {
            throw ParseException("Could not convert string to number.");
        }
        offset = endptr - base;
        return n;
    }

    template <typename T>
    std::string number_str() {
        bool valid;
        char* endptr;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        convert_to_number<T>(base + offset, valid, endptr);
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        if(!valid || endptr == base + offset) {
            throw ParseException("Could not convert string to number.");
        }
        ssize_t start = offset;
        offset = endptr - base;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        return std::string(base + start, static_cast<size_t>(offset - start));
    }
};

#endif
