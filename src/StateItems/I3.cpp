#include <ostream> // ostream
#include <utility> // pair

#include "StateItems/I3.hpp"

#include "Lemonbar.hpp"
#include "i3/ipc.hpp"
#include "i3/ipc_constants.hpp"
#include "utils/io.hpp"

void I3::workspace_event(std::unique_ptr<char[]> response) {
    JSON::Node json(response.get());
    std::string change(json["change"].string());

    auto& current = json["current"];
    uint8_t current_num = current["num"].number<uint8_t>();

    log() << change << " " << (int)current_num << std::endl;

    if(change == "init") {
        workspaces.init(current_num, current["name"].string());
    } else if(change == "focus") {
        workspaces.focus(current_num);
    } else if(change == "urgent") {
        // urgent_workspace(num, current["urgent"].boolean());
    } else if(change == "empty") {
        workspaces.empty(current_num);
    } else {
        log() << "Unhandled workspace event type: " << change << std::endl;
    }
}

void I3::window_event(std::unique_ptr<char[]> response) {
    JSON::Node json(response.get());
    std::string change = json["change"].string();

    auto& container = json["container"];
    uint64_t window_id = container["id"].number<uint64_t>();

    log() << "window"
          << " " << change << " " << window_id << std::endl;

    // if(change == "new")
    //     ; // i3.new_window(window_id, get_window_name(container));
    // else if(change == "title")
    //     ; // i3.rename_window(window_id, get_window_name(container));
    // else if(change == "focus")
    //     ; // i3.focus_window(window_id);
    // else if(change == "close")
    //     ; // i3.close_window(window_id);
    // else if(change == "fullscreen_mode")
    //     log() << "Ignoring fullscreen mode event" << endl;
    // else if(change == "move")
    //     log() << "Ignoring move event" << endl;
    // else
    //     log() << "Unhandled window event type: " << change << endl;
}

void I3::mode_event(std::unique_ptr<char[]> response) {
    JSON::Node json(response.get());
    mode.assign(json["change"].string());
}

std::pair<bool, bool> I3::handle_message(uint32_t type, std::unique_ptr<char[]> response) {
    switch(type) {
    case i3_ipc::reply_type::SUBSCRIBE:
        log() << "Subscribed to events - " << response.get() << std::endl;
        return {true, true};
    // case i3_ipc::event_type::OUTPUT:
    //     return output_event(response);
    case i3_ipc::event_type::WORKSPACE:
        workspace_event(std::move(response));
        return {true, true};
    case i3_ipc::event_type::WINDOW:
        window_event(std::move(response));
        return {true, true};
    case i3_ipc::event_type::MODE:
        mode_event(std::move(response));
        return {true, true};
    case i3_ipc::INVALID_TYPE:
        log() << "Invalid packet received." << std::endl;
        return {false, false};
    default:
        log() << "Unhandled event type: " << i3_ipc::type_to_string(type) << std::endl;
        return {true, true};
    }
}

std::pair<bool, bool> I3::update_raw() {
    // Pull and parse entire tree.
    // Update outputs, workspaces and windows accordingly.
    // Forced rerender every `cooldown` seconds.
    return {true, true};
}

std::pair<bool, bool> I3::handle_stream_data_raw(int fd) {
    uint32_t event_type;
    auto event_message = i3_ipc::read_message(fd, event_type);
    return handle_message(event_type, std::move(event_message));
}

void I3::print_raw(Lemonbar& bar, uint8_t display) { workspaces.print_raw(bar, display); }

I3::I3(JSON::Node const& item) : StateItem(item), command_socket(0), event_socket(0) {
    const auto get_socket_path_command = "i3 --get-socketpath";
    auto get_socket_path_pipe = run_command(get_socket_path_command, "r");
    char buffer[100];
    char* buffer_ptr = static_cast<char*>(buffer);
    fgets(buffer_ptr, 100, get_socket_path_pipe.get());
    auto const socket_path = std::string(buffer_ptr, strnlen(buffer_ptr, 100 - 1) - 1);
    event_socket = connect_to(socket_path);
    std::string abonnements = R"(["workspace","mode","output","window"])";
    i3_ipc::write_message(event_socket, i3_ipc::message_type::SUBSCRIBE, abonnements);
    register_event_socket(event_socket);
}

I3::~I3() { unregister_event_socket(event_socket); }
