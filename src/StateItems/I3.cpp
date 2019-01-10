#include <algorithm> // find_if, for_each
#include <ostream>   // ostream
#include <utility>   // pair

#include "StateItems/I3.hpp"

#include "Lemonbar.hpp"
#include "i3/ipc.hpp"
#include "i3/ipc_constants.hpp"
#include "utils/io.hpp"

void I3::query_tree() {
    std::unique_ptr<char[]> response = i3_ipc::query(command_socket, i3_ipc::message_type::GET_TREE);
    JSON::Node json(response.get());

    auto const& output_nodes = json["nodes"].array();
    for(auto const& output_node : output_nodes) {
        // Make all of these into displays.
        log() << output_node["name"].string() << std::endl;

        auto const& area_nodes = output_node["nodes"].array();
        // Here we only care about the `content`-node hence the find.
        auto const predicate = [](auto const& node) { return node["name"].string() == "content"; };
        auto const content_node_iter = std::find_if(area_nodes.begin(), area_nodes.end(), predicate);
        if(content_node_iter == area_nodes.end()) {
            log() << "Has the structure of the tree changed? Can't find `content` node." << std::endl;
            continue;
        }
        auto const& content_node = *content_node_iter;
        auto const& workspace_nodes = content_node["nodes"].array();
        for(auto const& workspace_node : workspace_nodes) {
            auto const workspace_num = workspace_node["num"].number<uint8_t>();
            auto const display_name = workspace_node["output"].string();
            auto const workspace_name = workspace_node["name"].string();
            workspaces.init(workspace_num, outputs.get_num(display_name), workspace_name);

            std::function<void(JSON::Node const&)> parse_tree_container =
                [this, &workspace_num, &parse_tree_container](JSON::Node const& container) {
                    auto const window_id = container["window"];
                    if(!window_id.exists()) {
                        auto const& con_nodes = container["nodes"].array();
                        std::for_each(con_nodes.begin(), con_nodes.end(), parse_tree_container);
                        return;
                    }

                    auto const& window_name = container["name"].string();
                    windows.open(window_id.number<uint64_t>(), workspace_num, window_name);
                };

            parse_tree_container(workspace_node);
        }
    }
}

void I3::workspace_event(std::unique_ptr<char[]> response) {
    JSON::Node json(response.get());
    std::string change(json["change"].string());

    auto const& current = json["current"];
    auto current_num = current["num"].number<uint8_t>();

    if(change == "init") {
        auto const display_name = current["output"].string();
        workspaces.init(current_num, outputs.get_num(display_name), current["name"].string());
    } else if(change == "focus") {
        workspaces.focus(current_num);
    } else if(change == "urgent") {
        workspaces.urgent(current_num, current["urgent"].boolean());
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
    log() << "Performing full tree update" << std::endl;
    query_tree();
    return {true, true};
}

std::pair<bool, bool> I3::handle_stream_data_raw(int fd) {
    uint32_t event_type;
    auto event_message = i3_ipc::read_message(fd, event_type);
    return handle_message(event_type, std::move(event_message));
}

void I3::print_raw(Lemonbar& bar, uint8_t display) { workspaces.print_raw(bar, display); }

I3::I3(JSON::Node const& item) : StateItem(item) {
    const auto get_socket_path_command = "i3 --get-socketpath";
    auto get_socket_path_pipe = run_command(get_socket_path_command, "r");
    char buffer[100];
    char* buffer_ptr = static_cast<char*>(buffer);
    fgets(buffer_ptr, 100, get_socket_path_pipe.get());
    auto const socket_path = std::string(buffer_ptr, strnlen(buffer_ptr, 100 - 1) - 1);

    outputs.add(0, "eDP1");
    outputs.add(1, "VGA1");
    outputs.add(2, "DP2-2");

    command_socket = connect_to(socket_path);
    event_socket = connect_to(socket_path);

    assert(command_socket.is_active());
    assert(event_socket.is_active());

    std::string abonnements = R"(["workspace","mode","output","window"])";
    i3_ipc::write_message(event_socket, i3_ipc::message_type::SUBSCRIBE, abonnements);
    register_event_socket(event_socket);
}

I3::~I3() { unregister_event_socket(event_socket); }
