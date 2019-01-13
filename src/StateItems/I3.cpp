#include <algorithm> // find_if, for_each
#include <map>
#include <ostream> // ostream
#include <regex>
#include <string>
#include <utility> // pair

#include <fmt/format.h>

#include "StateItems/I3.hpp"

#include "Lemonbar.hpp"
#include "i3/ipc.hpp"
#include "i3/ipc_constants.hpp"
#include "utils/i3_tree.hpp"
#include "utils/io.hpp"

std::string I3::get_window_name(JSON::Node const& container) {
    auto const name(container["name"].string("ERROR"));

    std::map<std::string, std::string> regex_to_icon{
        // Chrome.
        {".*(telegram).*chrom.*", "\uf268\ue217"},
        {".*(slack).*chrom.*", "\uf268\uf198"},
        {".*(github).*chrom.*", "\uf268\uf408"},
        {".*(gitlab).*chrom.*", "\uf268\uf296"},
        {".*(stack.*overflow).*chrom.*", "\uf268\uf16c"},
        {".*chrom.*", "\uf268"},

        // Desktop programs.
        {".*vlc.*", "\ufa7b"},
        {".*mumble.*", "\uf130"},

        // Vim (with filetype).
        {".*(\\.hpp|\\.cpp).*vim.*", "\ue62b\ufb71"},
        {".*(\\.ts|\\.tsx).*vim.*", "\ue62b\ufbe4"},
        {".*(\\.py).*vim.*", "\ue62b\ue235"},
        {".*(\\.js|\\.jsx).*vim.*", "\ue62b\ue781"},
        {".*(\\.rs).*vim.*", "\ue62b\ue7a8"},
        {".*(docker).*vim.*", "\ue62b\uf308"},
        {".*vim.*", "\ue62b"},

        // Shell commands.
        {".*make.*", "\uf423"},
        {".*psql.*", "\ue76e"},
        {".*htop.*", "\uf0e4"},
        {".*man.*", "\uf15c"},
        {".*zsh.*", "\uf120"},
    };
    for(auto const& pair : regex_to_icon) {
        std::regex regex(pair.first, std::regex_constants::icase);
        if(std::regex_match(name, regex)) {
            return pair.second;
        }
    }

    // std::cout << name.length() << " " << name << std::endl;
    if(name.length() <= 20) {
        return name;
    }
    // auto const class_name(container["window_properties"]["class"].string("ERROR"));
    // if(class_name.length() <= 20) {
    //     return class_name;
    // }
    return fmt::format("{}[…]", name.substr(0, 10));
}

void I3::query_tree() {
    std::unique_ptr<char[]> response = i3_ipc::query(command_socket, i3_ipc::message_type::GET_TREE);
    JSON::Node json(response.get());

    auto const& output_nodes = json["nodes"].array();
    for(auto const& output_node : output_nodes) {
        // Make all of these into displays except for `__i3`.
        if(output_node["name"].string() == "__i3") {
            continue;
        }

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
            auto const workspace_name = workspace_node["name"].string();
            auto const workspace_num = workspace_node["num"].number<uint8_t>();
            auto const display_name = workspace_node["output"].string();
            auto const display_num = outputs.get_num(display_name);
            workspaces.init(workspace_num, display_num, workspace_name);

            std::function<void(JSON::Node const&)> parse_tree_container =
                [&, this](JSON::Node const& container) {
                    auto const window_id_json = container["window"];
                    // If the container does not have a window id, then it is recursive.
                    if(!window_id_json.exists()) {
                        auto const& con_nodes = container["nodes"].array();
                        std::for_each(con_nodes.begin(), con_nodes.end(), parse_tree_container);
                        return;
                    }

                    // Create the actual window.
                    auto const window_id = window_id_json.number<uint64_t>();
                    auto const window_name = get_window_name(container);
                    windows.open(window_id, workspace_num, window_name);
                    workspaces.focus_window(workspace_num, window_id);
                    // If the window is focused, then the corresponding workspace is focused on the display.
                    if(container["focused"].boolean()) {
                        workspaces.focused = workspace_num;
                        outputs.focus_workspace(display_num, workspace_num);
                    }
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
        workspaces.focused = current_num;
        auto const display_name = current["output"].string();
        outputs.focus_workspace(outputs.get_num(display_name), current_num);
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
    uint64_t window_id = container["window"].number<uint64_t>();

    if(!windows.contains(window_id)) {
        auto const output_name = container["output"].string();
        auto const workspace_num = outputs.get_workspace_num_of_focused_on(output_name);
        windows.open(window_id, workspace_num, get_window_name(container));
    }

    if(change == "new") {
        // Pass.
    } else if(change == "title") {
        windows.title(window_id, get_window_name(container));
    } else if(change == "focus") {
        auto const workspace_num = windows.get_workspace_num(window_id);
        workspaces.focus_window(workspace_num, window_id);
    } else if(change == "move") {
        auto const output_name = container["output"].string();
        auto const workspace_num = outputs.get_workspace_num_of_focused_on(output_name);
        windows.move(window_id, workspace_num);
    } else if(change == "close") {
        auto const workspace_num = windows.get_workspace_num(window_id);
        windows.close(window_id);
        workspaces.focus_window(workspace_num, 0);
    }
    // else if(change == "fullscreen_mode")
    //     log() << "Ignoring fullscreen mode event" << endl;
    else {
        log() << "Unhandled window event type: " << change << std::endl;
    }
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

void I3::print_raw(Lemonbar& bar, uint8_t display) {
    auto const& style = Lemonbar::PowerlineStyle::round;
    for(auto const& workspace_entry : workspaces) {
        auto const& workspace = workspace_entry.second;
        if(workspace.display == display) {
            auto const& sep_right = Lemonbar::Separator::right;
            bool is_focused = workspace_entry.first == workspaces.focused;
            auto const& coloring = workspace.urgent
                                       ? Lemonbar::Coloring::urgent
                                       : is_focused ? Lemonbar::Coloring::active : Lemonbar::Coloring::inactive;
            bar.separator(sep_right, coloring, style);
            bar() << " " << workspace.name << " ";
            auto const focused_window = workspace.focused_window;
            if(focused_window != 0) {
                if(!windows.contains(focused_window)) {
                    bar() << " unknown focus ";
                } else {
                    auto const& name = windows[focused_window].name;
                    if(!is_unicode(name)) {
                        bar.separator(sep_right, style);
                        bar() << " " << name << " ";
                    } else {
                        bar() << name;
                    }
                }
            }
            bar.separator(sep_right, Lemonbar::Coloring::white_on_black, style);
        }
    }
}

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
