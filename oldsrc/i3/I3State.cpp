#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <tuple>
#include <utility>

#include <sys/socket.h>

#include "Application.hpp"
#include "output.hpp"
#include "util.hpp"
#include "I3State.hpp"
#include "i3-ipc-constants.hpp"
#include "i3-ipc.hpp"

using namespace std;
using BarWriter::Coloring;
using BarWriter::Separator;

void I3State::add_workspace(
    std::string const& output_name,
    uint8_t workspace_num,
    std::string const& workspace_name)
{
    Output& output = outputs.at(output_name);
    auto result = workspaces.emplace(
        piecewise_construct,
        forward_as_tuple(workspace_num),
        forward_as_tuple(workspace_num, workspace_name, output));

    auto& workspace = result.first->second;
    output.workspaces.emplace(workspace_num, std::ref(workspace));
}

I3State::I3State() : Logger("I3State")
{
    command_socket = 0;
    mode = "default";
}

void I3State::set_command_socket(int com_socket)
{
    command_socket = com_socket;
    get_outputs();
    get_workspaces();
}

/**
 * A simple algorithm for lexicographically sorting outputs.
 */
uint64_t make_output_index(uint32_t x, uint32_t y)
{
    return ((uint64_t)x << 32) | (uint64_t)y;
}

void I3State::get_outputs(void)
{
    auto buffer = query(command_socket, I3_IPC_MESSAGE_TYPE_GET_OUTPUTS);
    if(!buffer)
    {
        log() << "Failed to get outputs" << endl;
        return;
    }

    auto const array = JSON::Node(buffer.get()).array();

    uint8_t total_displays((uint8_t)array.size());
    log() << "Total outputs: " << (int)total_displays << endl;
    std::map<uint64_t, string> outputs_map;
    for(auto const disp : array)
    {
        if(disp["active"].boolean())
        {
            auto const rect = disp["rect"];
            auto output_index = make_output_index(
                rect["x"].number<uint32_t>(), rect["y"].number<uint32_t>());
            outputs_map[output_index] = disp["name"].string();
        }
    }

    uint8_t i = 0;
    for(auto const& kv_pair : outputs_map)
    {
        outputs.emplace(
            piecewise_construct,
            forward_as_tuple(kv_pair.second),
            forward_as_tuple(i, kv_pair.second));
        i++;
    }
}

void I3State::get_workspaces(void)
{
    auto buffer = query(command_socket, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES);
    if(!buffer)
    {
        log() << "Failed to get workspaces" << endl;
        return;
    }

    auto json = JSON::Node(buffer.get()).array();

    for(auto const workspace_json : json)
    {
        uint8_t num = workspace_json["num"].number<uint8_t>();
        string name(workspace_json["name"].string());
        add_workspace(workspace_json["output"].string(), num, name);
    }
}

void I3State::new_window(uint64_t id, std::string const& name)
{
    windows[id] = {id, name};
}

void I3State::rename_window(uint64_t id, std::string const& name)
{
    windows[id].name = name;
}

void I3State::focus_window(uint64_t id)
{
    // TODO focus the workspace on which this window is located.
}

void I3State::close_window(uint64_t id)
{
    windows.erase(id);
}

void I3State::init_workspace(uint8_t workspace_num, string const& name)
{
    // When a new workspace is initialized, we need to query the workspaces,
    // since the information on which output the new workspace is located is
    // not transmitted alongside the event.
    auto buffer = query(command_socket, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES);
    if(buffer)
    {
        auto const array = JSON::Node(buffer.get()).array();
        for(auto const workspace_json : array)
        {
            uint8_t num = workspace_json["num"].number<uint8_t>();
            if(num == workspace_num)
            {
                add_workspace(workspace_json["output"].string(), num, name);
                break;
            }
        }
    }
}

void I3State::focus_workspace(uint8_t workspace_num)
{
    focused_workspace = &workspaces.at(workspace_num);
    auto& output = focused_workspace->output;
    focused_output = &output;
    output.focused_workspace = focused_workspace;
}

void I3State::urgent_workspace(uint8_t workspace_num, bool urgent)
{
    workspaces.at(workspace_num).urgent = urgent;
}

void I3State::empty_workspace(uint8_t workspace_num)
{
    auto& workspace = workspaces.at(workspace_num);
    auto& output = workspace.output;
    output.workspaces.erase(workspace_num);
    if(output.focused_workspace == &workspace)
        output.focused_workspace = nullptr;
    workspaces.erase(workspace_num);
}

//
// void I3State::init_layout(void)
// {
//     get_outputs();
//     if(valid)
//         get_workspaces();
// }
//
//
// void I3State::update_from_tree(void)
// {
//     valid = false;
//
//     auto buffer = message(I3_IPC_MESSAGE_TYPE_GET_TREE);
//     if(buffer != nullptr)
//     {
//         char const* buffer_ptr = buffer.get();
//         auto json_uptr = JSON::parse(buffer_ptr);
//         auto const& root = *json_uptr;
//
//         // Root nodes are outputs.
//         update_outputs_from_tree(root["nodes"]);
//     }
// }
//
// void I3State::update_outputs_from_tree(JSON::Value const& root_children)
// {
//     for(auto& output_obj : root_children.as_vector())
//     {
//         // For each output tree ...
//         auto& output_name = (*output_obj)["name"]; // eDP1, __i3, LVDS0
//
//         // ... search for output_name in list of known outputs.
//         auto output_kv = outputs.find(output_name);
//         // If it is an actual display ...
//         if(output_kv != outputs.end())
//         {
//             // ... get the `Output` object (shared pointer).
//             auto const output = output_kv->second;
//             update_workspaces_from_tree(output, (*output_obj)["nodes"]);
//         }
//         // If the display configuration changed then we need to restart
//         // statorange completely, because we can't adapt to sudden output
//         // changes on the fly.
//     }
// }
//
// void I3State::update_workspaces_from_tree(shared_ptr<Output> output,
//                                           JSON::Value const& output_children)
// {
//     for(auto& node : output_children.as_vector())
//     {
//         string const& node_type = node->get("type");
//
//         // Outputs have different types of nodes. We only want
//         `con`tainers.
//         if(node_type.compare("con") == 0)
//         {
//             // Get workspaces of the current output.
//             auto& workspaces_arr = (*node)["nodes"];
//             for(auto& workspace_obj : workspaces_arr.as_vector())
//             {
//                 // For each workspace tree ...
//                 uint8_t workspace_num = (*workspace_obj)["num"];
//
//                 // ... get the Workspace object from Output object.
//                 auto& workspace = workspaces[workspace_num];
//                 update_windows_from_tree(workspace,
//                 (*workspace_obj)["nodes"]);
//             }
//         }
//     }
// }
//
// void I3State::update_windows_from_tree(shared_ptr<Workspace> workspace,
//                                        JSON::Value const& workspace_children)
// {
//     for(auto& node : workspace_children.as_vector())
//     {
//         if(node->get("focused"))
//             workspace->focused_window_id = node->get("id");
//     }
// }
