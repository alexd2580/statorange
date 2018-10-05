#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <stack>
#include <string>

// Convert to unsigned integer.
template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value>* = nullptr>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid, char*& endptr) {
    errno = 0;
    uint64_t result = strtoull(c_str, &endptr, 10);
    valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max();
    return static_cast<T>(result);
}

// Convert to signed integer.
template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value>* = nullptr>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid, char*& endptr) {
    errno = 0;
    int64_t result = strtoll(c_str, &endptr, 10);
    valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max() &&
            result >= std::numeric_limits<T>::min();
    return static_cast<T>(result);
}

// Convert to floating point.
template <typename T, typename std::enable_if_t<std::is_floating_point<T>::value>* = nullptr>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid, char*& endptr) {
    errno = 0;
    long double result = strtold(c_str, &endptr);
    valid = endptr != c_str && errno != ERANGE && result <= static_cast<long double>(std::numeric_limits<T>::max()) &&
            result >= static_cast<long double>(std::numeric_limits<T>::lowest());
    return static_cast<T>(result);
}

template <typename T>
T convert_to_number(const char* c_str, T default_value) {
    char* endptr = nullptr;
    bool valid = false;
    auto result = convert_to_number<T>(c_str, valid, endptr);
    return valid ? result : default_value;
}

template <typename T>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid) {
    char* endptr = nullptr;
    return convert_to_number<T>(c_str, valid, endptr);
}

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

// In a fail case errno is set appropriately.
bool has_input(int fd, int microsec = 0);

void store_string(char* dst, size_t dst_size, char* src, size_t src_size);

// NOLINTNEXTLINE: Desired call signature.
bool load_file(std::string const& name, std::string& content);

// NOLINTNEXTLINE: Desired call signature.
std::ostream& print_time(std::ostream& out, struct tm& ptm, char const* const format);

ssize_t read_all(int fd, char* buf, size_t count);
ssize_t write_all(int fd, char const* buf, size_t count);

std::string make_hex_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b);

class FileStream : public std::streambuf {
  private:
    const int fd;
    int buffered_char = EOF;

  public:
    explicit FileStream(int fd);
    explicit FileStream(FILE* f);

    FileStream(const FileStream&) = delete;
    FileStream(FileStream&&) = delete;
    FileStream& operator=(const FileStream&) = delete;
    FileStream& operator=(FileStream&&) = delete;

    ~FileStream() override = default;

    int underflow() override;
    int uflow() override;
    int overflow(int c) override;
};

using UniqueFile = std::unique_ptr<FILE, int (*)(FILE*)>;

UniqueFile open_file(std::string const& path);
UniqueFile run_command(std::string const& command, std::string const& mode);

template <typename Resource, typename _ = void>
class UniqueResource {
    using Release = std::function<_(Resource)>;

  protected:
    Resource resource;
    Release release;

  public:
    explicit UniqueResource(Resource new_resource, Release new_release)
        : resource(new_resource), release(new_release) {}

    UniqueResource(UniqueResource const& other) = delete;
    UniqueResource& operator=(UniqueResource const& other) = delete;

    UniqueResource(UniqueResource&& other) noexcept : resource(other.resource), release(other.release) {}
    UniqueResource& operator=(UniqueResource&& other) noexcept {
        release(resource);
        resource = other.resource;
        release = other.release;
    }

    virtual ~UniqueResource() { release(resource); }
};

class UniqueSocket final : public UniqueResource<int> {
  public:
    explicit UniqueSocket(int new_sockfd);
    operator int(); // NOLINT: Implicit `int()` conversions are expected behavior.
};

#endif
