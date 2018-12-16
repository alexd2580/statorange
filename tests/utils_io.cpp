#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <utility>

#include <csignal>
#include <cstdio>
#include <fcntl.h>

#include <unistd.h>

#include <bandit/bandit.h>
#include <fmt/format.h>

// Local import.
#include "utils.hpp"

#include "utils/io.hpp"

using namespace bandit;
using namespace snowhouse;

using namespace std::chrono_literals;

go_bandit([] {
    describe("has_input", [] {
        it("yields and returns true if input is available", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            assert_write(in, "whatever", 8);
            AssertThat(has_input(out), IsTrue());
            AssertThat(has_input(out, 1000000), IsTrue());
        });
        it("yields and returns false if no input is available", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            AssertThat(has_input(out), IsFalse());
            AssertThat(has_input(out, 100), IsFalse());
        });
        it("blocks until input becomes available", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            auto future = std::async(std::launch::async, [out] { return has_input(out, 1000000); });

            AssertThat(has_input(out, 100), IsFalse());
            AssertThat(future.wait_for(100ms), !Equals(std::future_status::ready));
            assert_write(in, "whatever", 8);
            AssertThat(future.get(), Equals(true));
        });
        it("returns true (!) if the pipe is closed remotely", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            AssertThat(has_input(out), IsFalse());

            assert_write(in, "whatever", 8);
            AssertThat(has_input(out), IsTrue());

            char buffer[100];
            read(out, buffer, 100);
            AssertThat(has_input(out), IsFalse());

            close(in);
            AssertThat(has_input(out), IsTrue());

            close(out);
            AssertThat(has_input(out), IsFalse());
        });
    });
    describe("read_all", [] {
        it("reads as much as specified", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            assert_write(in, "whatever", 8);

            char buffer[4];

            AssertThat(read_all(out, buffer, 4), Equals(4));
            AssertThat(std::string(buffer, 4), Equals("what"));
            AssertThat(read_all(out, buffer, 4), Equals(4));
            AssertThat(std::string(buffer, 4), Equals("ever"));
        });
        it("blocks until enough input is available", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            char buffer[100];
            memset(buffer, '\0', 100);

            auto future = std::async(std::launch::async, [out, &buffer] { return read_all(out, buffer, 20); });

            AssertThat(future.wait_for(100ms), !Equals(std::future_status::ready));
            assert_write(in, "tenlettersandmorestuff", 10);
            assert_write(in, "anotherfewtesttesttest", 20);

            AssertThat(future.get(), Equals(20));
            AssertThat(std::string(buffer, 21), Equals(std::string("tenlettersanotherfew", 21)));
            AssertThat(has_input(out), IsTrue());
        });
        it("TODO ignores EINTR, EAGAIN and EWOULDBLOCK", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);

            // Set write operation non-blocking.
            int prev_flags = fcntl(out, F_GETFL);
            fcntl(out, F_SETFL, prev_flags | O_NONBLOCK);

            // Test that a regular read call would yield with `EWOULDBLOCK`.
            errno = 0;
            char c;
            AssertThat(read(out, &c, 1), Equals(-1));
            AssertThat(errno, Equals(EWOULDBLOCK));

            char buffer[11];
            auto future = std::async(std::launch::async, [out, &buffer] { return read_all(out, buffer, 11); });
            AssertThat(future.wait_for(100ms), !Equals(std::future_status::ready));
            assert_write(in, "would block", 11);
            AssertThat(future.get(), Equals(11));
            AssertThat(std::string(buffer, 11), Equals("would block"));
        });
        it("stops reading on closed connections and closes the socket", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);

            char buffer[100];
            memset(buffer, '\0', 100);
            auto future = std::async(std::launch::async, [out, &buffer] { return read_all(out, buffer, 20); });

            assert_write(in, "tenlettersandmorestuff", 10);
            assert_write(in, "four", 4);
            AssertThat(future.wait_for(100ms), !Equals(std::future_status::ready));
            close(in);

            AssertThat(future.get(), Equals(14));
            AssertThat(std::string(buffer, 15), Equals(std::string("tenlettersfour", 15)));
            AssertThat(has_input(out), IsFalse());
        });
        it("throws on closed sockets", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            close(out);

            char c;
            AssertThrows(StreamException, read_all(out, &c, 1));
            AssertThat(LastException<StreamException>().what(),
                       Equals(fmt::format("Read returned -1 with errno set to {}", EBADF)));
        });
    });
    describe("write_all", [] {
        it("writes as much as specified", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);

            AssertThat(write_all(in, "whatever", 4), Equals(4));
            AssertThat(write_all(in, "everyday", 4), Equals(4));
            assert_read<8>(out, "whatever");
        });
        it("blocks until everything can be written", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);

            // Get the pipe size.
            int fcntl_res = fcntl(in, F_GETPIPE_SZ);
            auto pipe_size = static_cast<uint32_t>(fcntl_res);

            // Write `pipe_size` bytes into the stream.
            UniqueResource<char*> empty_data(new char[pipe_size], [](char* ptr) { delete[] ptr; });
            AssertThat(write_all(in, empty_data.get(), pipe_size), Equals(pipe_size));

            // Launch a future which tries to push 17 more bytes into the pipe.
            auto future = std::async(std::launch::async, [in] { return write_all(in, "exceeds page size", 17); });

            // It should not succeed...
            AssertThat(future.wait_for(100ms), !Equals(std::future_status::ready));
            // ... until at least some data from the pipe is read.
            AssertThat(read(out, empty_data.get(), 10), Equals(10));
            AssertThat(future.wait_for(100ms), !Equals(std::future_status::ready));
            AssertThat(read(out, empty_data.get(), pipe_size - 10), Equals(pipe_size - 10));
            AssertThat(future.get(), Equals(17));
            assert_read<17>(out, "exceeds page size");
        });
        it("TODO ignores EINTR, EAGAIN and EWOULDBLOCK", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);

            // Set write operation non-blocking.
            int prev_flags = fcntl(in, F_GETFL);
            fcntl(in, F_SETFL, prev_flags | O_NONBLOCK);

            // Get the pipe size.
            int fcntl_res = fcntl(in, F_GETPIPE_SZ);
            auto pipe_size = static_cast<uint32_t>(fcntl_res);

            // Write `pipe_size` bytes into the stream.
            UniqueResource<char*> empty_data(new char[pipe_size], [](char* ptr) { delete[] ptr; });
            AssertThat(write_all(in, empty_data.get(), pipe_size), Equals(pipe_size));

            // Test that a regular write call would yield with `EWOULDBLOCK`.
            errno = 0;
            AssertThat(write(in, "c", 1), Equals(-1));
            AssertThat(errno, Equals(EWOULDBLOCK));

            auto future = std::async(std::launch::async, [in] { return write_all(in, "would block", 11); });
            AssertThat(future.wait_for(100ms), !Equals(std::future_status::ready));
            AssertThat(read(out, empty_data.get(), pipe_size), Equals(pipe_size));
            AssertThat(future.get(), Equals(11));
            assert_read<11>(out, "would block");
        });
        signal(SIGPIPE, SIG_IGN);
        it("stops writing and closes the socket when the pipe is closed remotely", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);

            AssertThat(write_all(in, "something", 9), Equals(9));
            assert_read<9>(out, "something");
            close(out);
            AssertThat(write_all(in, "else", 4), Equals(0));
        });
        it("throws on closed sockets", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            close(in);

            AssertThrows(StreamException, write_all(in, "a", 1));
            AssertThat(LastException<StreamException>().what(),
                       Equals(fmt::format("Write returned -1 with errno set to {}", EBADF)));
        });
        signal(SIGPIPE, SIG_DFL);
    });
    describe("UniqueSocket", [] {
        it("closes the supplied socket on destruction", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            assert_write(in, "whatever", 8);
            {
                UniqueSocket us(out);
                assert_read<4>(out, "what");
            }
            char c;
            AssertThat(read(out, &c, 1), Equals(-1));
            AssertThat(errno, Equals(EBADF));
        });
        it("returns the supplied socket when cast to int", [] {
            int in, out;
            auto unique_sockets = make_pipe(in, out);
            UniqueSocket in_s(in);
            UniqueSocket out_s(out);
            AssertThat(static_cast<int>(in_s), Equals(in));
            AssertThat(static_cast<int>(out_s), Equals(out));
        });
    });
    describe("load_file", [] {
        it("reads a file into a string", [] {
            auto temp_file = make_file();
            std::string expected_content = "Example\rData\nMulti\r\nline";
            write_to_file(temp_file.get(), expected_content);

            std::string content;
            AssertThat(load_file(temp_file.get(), content), Equals(true));
            AssertThat(content, Equals(expected_content));
        });
        it("returns false if the file is not readable", [] {
            auto temp_filename = make_filename();
            std::string content;
            AssertThat(load_file(temp_filename, content), Equals(false));
            AssertThat(content.length(), Equals(0));
        });
        it("TODO returns false if the file is bad after reading", [] {
            AssertThat(false, IsTrue());
            AssertThat(false, IsTrue());
        });
    });
    describe("run_command", [] {
        it("executes a command and returns a FILE*-like object to its output stream", [] {
            std::string command(R"(echo "Hello World!")");
            auto result = run_command(command.c_str(), "r");
            char buffer[14];
            fgets(buffer, 14, result.get());
            AssertThat(std::string(buffer, 14), Equals(std::string("Hello World!\n\0", 14)));
        });
    });
    describe("open_file", [] {
        it("opens a readable FILE* to the supplied file", [] {
            std::string filename = make_filename();
            write_to_file(filename, "Yay, some text");

            auto file = open_file(filename);
            char buffer[15];
            fgets(buffer, 15, file.get());
            AssertThat(std::string(buffer, 14), Equals("Yay, some text"));
        });
        it("closes the file when its result is destroyed", [] {
            std::string filename = make_filename();
            write_to_file(filename, "Yay, some text");

            FILE* file_ptr;
            {
                auto file = open_file(filename.c_str());
                file_ptr = file.get();
                char buffer[15];
                fgets(buffer, 15, file.get());
                AssertThat(std::string(buffer, 14), Equals("Yay, some text"));
            }

            errno = 0;
            AssertThat(ftell(file_ptr), Equals(-1));
            AssertThat(errno, Equals(EBADF));
        });
    });
    describe("FileStream", [] {
        it("opens a readable streambuf to to the supplied file descriptor", [] {
            AssertThat(false, IsTrue());
            AssertThat(false, IsTrue());
        });
        it("opens a readable streambuf to to the supplied FILE*", [] {
            AssertThat(false, IsTrue());
            AssertThat(false, IsTrue());
        });
        it("opens a readable streambuf to to the supplied UniqueSocket", [] {
            AssertThat(false, IsTrue());
            AssertThat(false, IsTrue());
        });
        it("opens a readable streambuf to to the supplied UniqueFile", [] {
            AssertThat(false, IsTrue());
            AssertThat(false, IsTrue());
        });
    });
    describe("print_time", [] {
        it("prints the given time in the supplied format", [] {
            AssertThat(false, IsTrue());
            AssertThat(false, IsTrue());
        });
    });
    describe("print_used_memory", [] {
        it("prints the given time in the supplied format", [] {
            AssertThat(false, IsTrue());
            AssertThat(false, IsTrue());
        });
    });
});

/*
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
if(endptr == base + offset) {
    std::cerr << "Could not convert string to number" << std::endl;
    exit(1);
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
ssize_t start = offset;
offset = endptr - base;
// NOLINTNEXTLINE: Pointer arithmetic intended.
return std::string(base + start, static_cast<size_t>(offset - start));
}
};
*/
/*

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

#endif*/
