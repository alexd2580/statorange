#ifndef STATEITEMS_I3_HPP
#define STATEITEMS_I3_HPP

#include <string>
#include <utility>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"
#include "utils/io.hpp"
#include "utils/i3_tree.hpp"

class I3 final : public StateItem {
  private:
    UniqueSocket command_socket;
    UniqueSocket event_socket;

    // i3-mode, defined in the i3-config.
    std::string mode;

    I3Tree tree;

    static std::string get_window_name(JSON::Node const& container);

    void query_tree();

    void workspace_event(std::unique_ptr<char[]> response);
    void window_event(std::unique_ptr<char[]> response);
    void mode_event(std::unique_ptr<char[]> response);

    std::pair<bool, bool> handle_message(uint32_t type, std::unique_ptr<char[]> response);

    std::pair<bool, bool> update_raw() override;
    std::pair<bool, bool> handle_stream_data_raw(int fd) override;
    void print_raw(Lemonbar&, uint8_t) override;

  public:
    explicit I3(JSON::Node const& item);

    I3(I3 const& other) = delete;
    I3& operator=(I3 const& other) = delete;
    I3(I3&& other) = delete;
    I3& operator=(I3&& other) = delete;

    ~I3() override;
};

#endif
