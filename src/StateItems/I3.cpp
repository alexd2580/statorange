#include <algorithm> // find_if, for_each
#include <cassert>   // assert
#include <map>
#include <optional>
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
    auto const name(container["name"].string("\uf7d5"));

    std::map<std::string, std::string> regex_to_icon{
        // Chrome.
        {".*(telegram).*chrom.*", "\uf268\ue217"},
        {".*(slack).*chrom.*", "\uf268\uf198"},
        {".*(github).*chrom.*", "\uf268\uf408"},
        {".*(gitlab).*chrom.*", "\uf268\uf296"},
        {".*(stack.*overflow).*chrom.*", "\uf268\uf16c"},
        {".*(youtube).*chrom.*", "\uf268\uf16a"},
        {".*(jira).*chrom.*", "\uf268\uf802"},
        {".*(paypal).*chrom.*", "\uf268\uf1ed"},
        {".*(gmail).*chrom.*", "\uf268\uf7aa"},
        {".*(amazon).*chrom.*", "\uf268\uf270"},
        {".*(google).*chrom.*", "\uf268\uf1a0"},
        {".*chrom.*", "\uf268"},

        // Desktop programs.
        {".*vlc.*", "\ufa7b"},
        {".*mumble.*", "\uf130"},
        {".*volume control.*", "\uf028"},
        {".*telegram.*", "\ue217"},

        // Vim (with filetype).
        {".*(\\.hpp|\\.cpp).*vim.*", "\ue62b\ufb71"},
        {".*(\\.h|\\.c).*vim.*", "\ue62b\ufb70"},
        {".*(\\.ts|\\.tsx).*vim.*", "\ue62b\ufbe4"},
        {".*(\\.py).*vim.*", "\ue62b\ue235"},
        // {".*(\\.js^o|\\.jsx).*vim.*", "\ue62b\ue781"},
        {".*(\\.js^o|\\.jsx).*vim.*", "\ue62b\ue74e"},
        {".*(\\.json).*vim.*", "\ue62b\ufb25"},
        {".*(\\.rs).*vim.*", "\ue62b\ue7a8"},
        {".*(docker).*vim.*", "\ue62b\uf308"},
        {".*vim.*", "\ue62b"},

        // Shell commands.
        {".*make.*", "\uf423"},
        {".*psql.*", "\ue76e"},
        {".*htop.*", "\uf0e4"},
        {".*man.*", "\uf15c"},
        {".*docker.*", "\uf308"},
        {".*npm.*", "\ue71e"},
        {".*irssi.*", "\uf292"},
        {".*gdb.*", "\uf423"},
        {".*zsh.*", "\uf120"},
        {".*@.*: ~.*", "\uf120"},
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
    tree.reset();
    std::unique_ptr<char[]> response = i3_ipc::query(command_socket, i3_ipc::message_type::GET_TREE);
    JSON::Node json(response.get());

    auto const& output_nodes = json["nodes"].array();
    for(auto const& output_node : output_nodes) {
        // Make all of these into displays except for `__i3`.
        auto const& output_name = output_node["name"].string();
        if(output_name == "__i3") {
            continue;
        }

        log() << "Found output '" << output_name << "'" << std::endl;

        if(tree.outputs.contains(output_name)) {
            log() << "  Output already exists. Skipping." << std::endl;
            continue;
        }
        auto const& rect = output_node["rect"];
        auto& output = tree.display_add(rect["x"].number<int>(), rect["y"].number<int>(), output_name);

        auto const& area_nodes = output_node["nodes"].array();
        // Here we only care about the `content`-node hence the find.
        auto const is_content = [](auto const& node) { return node["name"].string() == "content"; };
        auto const content_node_iter = std::find_if(area_nodes.begin(), area_nodes.end(), is_content);
        if(content_node_iter == area_nodes.end()) {
            log() << "  Can't find `content` node. Skipping." << std::endl;
            continue;
        }
        auto const& workspace_nodes = (*content_node_iter)["nodes"].array();
        for(auto const& workspace_node : workspace_nodes) {
            auto const workspace_name = workspace_node["name"].string();
            auto const workspace_num = workspace_node["num"].number<uint8_t>();

            log() << fmt::format("  Found workspace '{}' ({})", workspace_name, workspace_num) << std::endl;
            auto& workspace = tree.workspace_init(workspace_num, output_name, workspace_name);

            // Temporarily setting focus to the new workspace.
            output.visible_workspace = &workspace;

            auto focused_container = std::optional(workspace_node["id"].number<uint64_t>());

            std::function<void(JSON::Node const&)> parse_tree_container = [&, this](JSON::Node const& container) {
                auto const id = container["id"].number<uint64_t>();
                auto const is_focused = id == focused_container;
                if(is_focused) {
                    auto const& focus = container["focus"].array();
                    if(focus.size() != 0) {
                        focused_container = focus[0].number<uint64_t>();
                    }
                }

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

                log() << fmt::format("    Found window '{}' ({}) => '{}'", container["name"].string("\uf7d5"),
                                     window_id, window_name)
                      << std::endl;

                tree.window_open(window_id, output_name, window_name);
                if(is_focused) {
                    log() << "      Window is focused" << std::endl;
                    tree.workspace_focus(workspace_num, output_name);
                    tree.window_focus(window_id);
                }
            };

            parse_tree_container(workspace_node);
        }
    }

    // TODO set visibility properly.
}

void I3::workspace_event(std::unique_ptr<char[]> response) {
    JSON::Node json(response.get());
    std::string change(json["change"].string());

    auto const& current = json["current"];
    auto current_num = current["num"].number<uint8_t>();

    // std::cout << "Workspace " << change << " " << (int)current_num << std::endl;
    // std::cout << response.get() << std::endl << std::endl;

    if(change == "init") {
        auto const output_name = current["output"].string();
        auto const workspace_name = current["name"].string();
        log() << fmt::format("Initializing workspace {} ({}) on display {}", workspace_name, current_num, output_name)
              << std::endl;
        tree.workspace_init(current_num, output_name, workspace_name);
    } else if(change == "focus") {
        auto const output_name = current["output"].string();
        log() << fmt::format("Focusing workspace {} on display {}", current_num, output_name) << std::endl;
        tree.workspace_focus(current_num, output_name);
    } else if(change == "urgent") {
        log() << fmt::format("Urgent-ing workspace {}", current_num) << std::endl;
        tree.workspace_urgent(current_num, current["urgent"].boolean());
    } else if(change == "move") {
        // Requery tree until https://github.com/i3/i3/pull/3597#partial-pull-merging
        // is merged.
        query_tree();
    } else if(change == "empty") {
        log() << fmt::format("Emptying workspace {}", current_num) << std::endl;
        tree.workspace_empty(current_num);
    } else {
        log() << "Unhandled workspace event type: [" << change << "]" << std::endl;
    }
}

void I3::window_event(std::unique_ptr<char[]> response) {
    JSON::Node json(response.get());
    std::string change = json["change"].string();

    // std::cout << "Window " << change << std::endl;
    // std::cout << response.get() << std::endl << std::endl;

    auto& container = json["container"];
    uint64_t window_id = container["window"].number<uint64_t>();

    if(!tree.windows.contains(window_id)) {
        auto const& output_name = container["output"].string();
        auto const& window_name = get_window_name(container);
        log() << fmt::format("Opening window {} ({}) on display {}", window_name, window_id, output_name) << std::endl;
        tree.window_open(window_id, output_name, window_name);
    }

    if(change == "new") {
        // Noop. Added for structural consistency.
        tree.window_new();
    } else if(change == "title") {
        auto const& window_name = get_window_name(container);
        log() << fmt::format("Changing title of window {} to {}", window_id, window_name) << std::endl;
        tree.window_title(window_id, window_name);
    } else if(change == "focus") {
        log() << fmt::format("Focusing window {}", window_id) << std::endl;
        tree.window_focus(window_id);
    } else if(change == "urgent") {
        log() << fmt::format("Urgent-int window {}", window_id) << std::endl;
        tree.window_urgent(window_id, container["urgent"].boolean());
    } else if(change == "move") {
        auto const output_name = container["output"].string();
        log() << fmt::format("Moving window {} to {}", window_id, output_name) << std::endl;
        tree.window_move(window_id, output_name);
    } else if(change == "close") {
        log() << fmt::format("Closing window {}", window_id) << std::endl;
        tree.window_close(window_id);
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
    case i3_ipc::event_type::OUTPUT:
        // std::cout << response.get() << std::endl;
        return {true, true};
        // return output_event(response);
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
    auto const& res = handle_message(event_type, std::move(event_message));

    // Handle subsequent messages.
    if(!has_input(fd)) {
        return res;
    }

    auto const& res2 = handle_stream_data_raw(fd);
    return {res.first && res2.first, res.second || res2.second};
}

void I3::print_raw(Lemonbar& bar, uint8_t display) {
    auto const& output = *(tree.outputs.iterableFromIndex1().cbegin() + (int)display);
    auto const& style = Lemonbar::PowerlineStyle::round;
    for(auto const& workspace_entry : tree.workspaces) {
        auto const& workspace = workspace_entry.second;
        if(workspace.output != &output) {
            continue;
        }

        using C = Lemonbar::Coloring;
        using S = Lemonbar::Separator;

        bool focused = tree.workspaces.is_focused(workspace);
        bool urgent_is_focused = workspace.focused_window == workspace.last_urgent_window;
        bool active_is_urgent = workspace.urgent && urgent_is_focused;
        auto const& coloring = active_is_urgent ? C::urgent : focused ? C::active : C::inactive;

        bar.separator(S::right, coloring, style);
        bar() << " " << workspace.name << " ";
        auto focused_window = workspace.focused_window;
        if(focused_window != nullptr) {
            auto const& name = focused_window->name;
            if(!is_unicode(name)) {
                bar.separator(S::right, style);
                bar() << " " << name << " ";
            } else {
                bar() << name;
            }
        }

        if(workspace.urgent && !urgent_is_focused) {
            bar.separator(S::right, C::urgent, style);
            auto const& name = workspace.last_urgent_window->name;
            if(!is_unicode(name)) {
                bar.separator(S::right, style);
                bar() << " " << name << " ";
            } else {
                bar() << name;
            }
        }
        bar.separator(S::right, C::white_on_black, style);
    }
}

I3::I3(JSON::Node const& item) : StateItem(item) {
    const auto get_socket_path_command = "i3 --get-socketpath";
    auto get_socket_path_pipe = run_command(get_socket_path_command, "r");
    char buffer[100];
    char* buffer_ptr = static_cast<char*>(buffer);
    fgets(buffer_ptr, 100, get_socket_path_pipe.get());
    auto const socket_path = std::string(buffer_ptr, strnlen(buffer_ptr, 100 - 1) - 1);

    command_socket = connect_to(socket_path);
    event_socket = connect_to(socket_path);

    assert(command_socket.is_active());
    assert(event_socket.is_active());

    query_tree();

    std::string abonnements = R"(["workspace","mode","output","window"])";
    i3_ipc::write_message(event_socket, i3_ipc::message_type::SUBSCRIBE, abonnements);
    register_event_socket(event_socket);
}

I3::~I3() { unregister_event_socket(event_socket); }
