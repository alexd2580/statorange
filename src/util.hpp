#ifndef __STRUTIL_LOL___
#define __STRUTIL_LOL___

#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
#include <stack>
#include <string>

// Convert to unsigned integer.
template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value>* = nullptr>
T convertToNumber(const char* c_str, T default_value) {
    char* endptr = nullptr;
    unsigned long long result = strtoull(c_str, &endptr, 10);
    bool valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max();
    return valid ? (T)result : default_value;
}

// Convert to signed integer.
template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value>* = nullptr>
T convertToNumber(const char* c_str, T default_value) {
    char* endptr = nullptr;
    long long result = strtoll(c_str, &endptr, 10);
    bool valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max() &&
                 result >= std::numeric_limits<T>::min();
    return valid ? (T)result : default_value;
}

// Convert to floating point.
template <typename T, typename std::enable_if_t<std::is_floating_point<T>::value>* = nullptr>
T convertToNumber(const char* c_str, T default_value) {
    char* endptr = nullptr;
    long double result = strtold(c_str, &endptr);
    bool valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max() &&
                 result >= std::numeric_limits<T>::lowest();
    return valid ? (T)result : default_value;
}

char next(char const*& string);
void whitespace(char const*& string);
void nonspace(char const*& string);
double number(char const*& string);
std::string number_str(char const*& string);
std::string escaped_string(char const*& string);

// In a fail case errno is set appropriately.
bool has_input(int fd, int microsec = 0);

void store_string(char* dst, size_t dst_size, char* src, size_t src_size);

bool load_file(std::string const& name, std::string& content);

std::ostream& print_time(std::ostream& out, struct tm& ptm, char const* const format);

ssize_t read_all(int fd, char* buf, size_t count);
ssize_t write_all(int fd, char const* buf, size_t count);

std::string make_hex_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b);

class FileStream : public std::streambuf {
  private:
    const int fd;
    int buffered_char = EOF;

    FileStream(const FileStream&) = delete;
    FileStream(FileStream&&) = delete;
    FileStream& operator=(const FileStream&) = delete;
    FileStream& operator=(FileStream&&) = delete;

  public:
    explicit FileStream(int fd);
    explicit FileStream(FILE* f);
    virtual ~FileStream() override = default ;

    virtual int underflow() override;
    virtual int uflow() override;
    virtual int overflow(int c) override;
};

std::unique_ptr<FILE, int(*)(FILE*)> run_command(std::string const& command, std::string const& mode);

#endif
