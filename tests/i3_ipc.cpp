#include <fstream>
#include <future>
#include <string>
#include <vector>

#include <cmath>
#include <csignal>
#include <cstdint>

#include <fmt/format.h>

#include <bandit/bandit.h>

// Local import.
#include "Logger.hpp"
#include "utils.hpp"

#include "i3/ipc.hpp"
#include "i3/ipc_constants.hpp"

using namespace bandit;
using namespace snowhouse;

static std::ofstream null;

go_bandit([] {
    describe("i3 ipc", [] {
        before_each([]() {
            null.open("/dev/null", std::ofstream::out);
            Logger::set_default_ostream(null);
        });
        after_each([]() {
            null.close();
            Logger::set_default_ostream(std::cout);
        });
        describe("type_to_string", [] {
            std::vector<std::pair<uint32_t, std::string>> conversions = {
                {i3_ipc::reply_type::COMMAND, "reply_type::COMMAND"},
                {i3_ipc::reply_type::OUTPUTS, "reply_type::OUTPUTS"},
                {i3_ipc::reply_type::BAR_CONFIG, "reply_type::BAR_CONFIG"},
                {i3_ipc::event_type::WORKSPACE, "event_type::WORKSPACE"},
                {i3_ipc::event_type::WINDOW, "event_type::WINDOW"},
                {i3_ipc::event_type::BINDING, "event_type::BINDING"}};
            for(auto& test_case : conversions) {
                it(fmt::format("properly converts {}", test_case.second),
                   [&] { AssertThat(i3_ipc::type_to_string(test_case.first), Equals(test_case.second)); });
            }
        });
        describe("write_message", [] {
            it("successfully reads a message from the stream", [] {
                int in, out;
                auto unique_sockets = make_pipe(in, out);

                AssertThat(i3_ipc::write_message(in, i3_ipc::message_type::COMMAND, "echo lol"), IsTrue());
                assert_write(in, "i3-ipc", 6);
                AssertThat(write(in, &i3_ipc::message_type::COMMAND, 4), Equals(4));
                const uint32_t payload_size = 8;
                AssertThat(write(in, &payload_size, 4), Equals(4));
                assert_write(in, "echo lol", 4);

                uint32_t result_type;
                auto const result = i3_ipc::read_message(out, result_type);
                AssertThat(result_type, Equals(i3_ipc::message_type::COMMAND));
                AssertThat(result.get(), Is().Not().EqualTo(nullptr));
                AssertThat(std::string(result.get()), Equals("echo lol"));
            });
        });
        describe("query", [] {
            it("successfully writes a message to the stream and reads the output", [] {
                int server, client;
                auto unique_sockets = make_bidirectional_pipe(server, client);

                auto future = std::async(std::launch::async, [&client] {
                    uint32_t type;
                    auto message = i3_ipc::read_message(client, type);
                    AssertThat(type, Equals(i3_ipc::message_type::GET_TREE));
                    AssertThat(message, Equals(nullptr));
                    auto const reply_message = "Replying to ipc command";
                    AssertThat(i3_ipc::write_message(client, type, reply_message), IsTrue());
                });

                auto const result = i3_ipc::query(server, i3_ipc::message_type::GET_TREE);
                AssertThat(result.get(), Equals("Replying to ipc command"));
                future.wait();
            });
            it("throws when writing fails due to a network exception", [] {
                int server, client;
                // Instantly close the sockets to make them invalid.
                make_bidirectional_pipe(server, client);

                AssertThrows(StreamException, i3_ipc::query(server, i3_ipc::message_type::GET_TREE));
                AssertThat(LastException<StreamException>().what(), Equals("Write returned -1 with errno set to 9"));
            });
            signal(SIGPIPE, SIG_IGN);
            it("responds with an empty message when writing fails due to EPIPE", [] {
                int server, client;
                auto unique_sockets = make_bidirectional_pipe(server, client);
                // Only close the reading (server reads from client) socket.
                close(client);

                auto const result = i3_ipc::query(server, i3_ipc::message_type::GET_TREE);
                AssertThat(result.get(), Equals(nullptr));
            });
            signal(SIGPIPE, SIG_DFL);
            it("responds with an empty message when reading fails", [] {
                int server, client;
                auto unique_sockets = make_bidirectional_pipe(server, client);

                auto future = std::async(std::launch::async, [&client] {
                    uint32_t type;
                    auto message = i3_ipc::read_message(client, type);
                    AssertThat(type, Equals(i3_ipc::message_type::GET_TREE));
                    AssertThat(message, Equals(nullptr));
                    auto const reply_message = "Replying to ipc command";
                    assert_write(client, reply_message, 23);
                });

                auto const result = i3_ipc::query(server, i3_ipc::message_type::GET_TREE);
                AssertThat(result.get(), Equals(nullptr));
                future.wait();
            });
        });
    });
});
