#ifndef UTILS_IO_HPP
#define UTILS_IO_HPP

#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

#include <cstddef>
#include <cstdio>

#include <sys/socket.h>

#include "Address.hpp"
#include "utils/exception.hpp"
#include "utils/resource.hpp"

// Returns true if input becomes available on a non-closed socket.
// Returns true if the pipe has been closed remotely.
bool has_input(int fd, int microsec = 0);

DEFINE_EXCEPTION_CLASS(StreamException)

// Reads `count` bytes from `fd` and stores them into `buf`.
// Closes `fd` when EOS is reached.
// Returns the number of read bytes.
ssize_t read_all(int fd, char* buf, size_t count) noexcept(false);

// Writes `count` bytes from `buf` and to `fd`.
// Closes `fd` when EPIPE is received.
// Returns the number of wrote bytes.
ssize_t write_all(int fd, char const* buf, size_t count) noexcept(false);

class UniqueSocket final : public UniqueResource<int> {
  public:
    explicit UniqueSocket();
    explicit UniqueSocket(int new_sockfd);
    operator int() const; // NOLINT: Implicit `int()` conversions are expected behavior.
};

// NOLINTNEXTLINE: Desired call signature.
bool load_file(std::string const& name, std::string& content);

using UniqueFile = std::unique_ptr<FILE, int (*)(FILE*)>;

UniqueFile run_command(std::string const& command, std::string const& mode);
UniqueFile open_file(std::string const& path);

template <typename Resource>
class FileStream {};

#define DECLARE_FILE_STREAM(TYPE, RESOURCE_VARIABLE_NAME, RESOURCE_INITIALIZER, FD_INITIALIZER)                        \
    template <>                                                                                                        \
    class FileStream<TYPE> : public std::streambuf {                                                                   \
      private:                                                                                                         \
        /* This resource will be freed in the destructor. This resource will be freed in the destructor.  */           \
        TYPE const resource;                                                                                           \
        int const fd;                                                                                                  \
        int buffered_char = EOF;                                                                                       \
                                                                                                                       \
      public:                                                                                                          \
        explicit FileStream(TYPE RESOURCE_VARIABLE_NAME) : resource(RESOURCE_INITIALIZER), fd(FD_INITIALIZER) {}       \
                                                                                                                       \
        FileStream(const FileStream&) = delete;                                                                        \
        FileStream(FileStream&&) = delete;                                                                             \
        FileStream& operator=(const FileStream&) = delete;                                                             \
        FileStream& operator=(FileStream&&) = delete;                                                                  \
                                                                                                                       \
        ~FileStream() override = default;                                                                              \
                                                                                                                       \
        int underflow() override {                                                                                     \
            if(buffered_char == EOF) {                                                                                 \
                char c;                                                                                                \
                buffered_char = read_all(fd, &c, 1) == 1 ? static_cast<int>(c) : EOF;                                  \
            }                                                                                                          \
            return buffered_char;                                                                                      \
        }                                                                                                              \
                                                                                                                       \
        int uflow() override {                                                                                         \
            if(buffered_char == EOF) {                                                                                 \
                char c;                                                                                                \
                return read_all(fd, &c, 1) == 1 ? static_cast<int>(c) : EOF;                                           \
            }                                                                                                          \
            auto c = static_cast<char>(buffered_char);                                                                 \
            buffered_char = EOF;                                                                                       \
            return c;                                                                                                  \
        }                                                                                                              \
                                                                                                                       \
        int overflow(int i) override {                                                                                 \
            auto c = static_cast<char>(i);                                                                             \
            return write_all(fd, &c, 1) == 1 ? i : EOF;                                                                \
        }                                                                                                              \
    };

// We ignore the `unused-private-field` warning, because we use this field to
// hold ownership of the resource and free it on destruction.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"

// Non-owning file streams.
DECLARE_FILE_STREAM(int, new_fd, new_fd, new_fd)
DECLARE_FILE_STREAM(FILE*, new_file, new_file, fileno(new_file))

// Owning file streams.
DECLARE_FILE_STREAM(UniqueSocket, us, std::move(us), static_cast<int>(resource))
DECLARE_FILE_STREAM(UniqueFile, uf, std::move(uf), fileno(resource.get()))

#pragma clang diagnostic pop

// NOLINTNEXTLINE: Desired call signature.
std::ostream& print_time(std::ostream& out, struct tm& ptm, char const* const format);
std::ostream& print_used_memory(std::ostream& out, uint64_t used, uint64_t total);

UniqueSocket connect_to(std::string const& path, unsigned int port = 0, int domain = AF_LOCAL,
                        std::string const& interface = "", bool verbose = true);
UniqueSocket connect_to(Address const& path, int domain = AF_LOCAL, std::string const& interface = "",
                        bool verbose = true);

#endif
