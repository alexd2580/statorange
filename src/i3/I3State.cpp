#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <sys/socket.h>

#include "../Application.hpp"
#include "../util.hpp"
#include "I3State.hpp"
#include "i3-ipc-constants.hpp"
#include "i3-ipc.hpp"

using namespace std;

/******************************************************************************/
/******************************************************************************/

struct OutputRect
{
    std::string name;
    int x;
    int y;

    OutputRect(JSON const& output)
    {
        name.assign(output["name"]);
        auto& rect = output["rect"];
        x = rect["x"];
        y = rect["y"];
    }

    // Returns `true` if `this` is at a "smaller" position then `other`.
    __attribute__((pure)) bool operator<(OutputRect const& other) const
    {
        return x < other.x || (x == other.x && y < other.y);
    }
};

/******************************************************************************/
/******************************************************************************/

I3State::I3State(string const& path)
    : StateItem("[I3State]") push_socket(init_socket(path))
{
    mode = "default";
    valid = false;
}

I3State::~I3State(void)
{
    focused_output.reset();
    for(auto kv_pair0 : outputs)
    {
        auto& output = *kv_pair0.second;
        output.focused_workspace.reset();
        for(auto kv_pair1 : output.workspaces)
        {
            auto& workspace = *kv_pair1.second;
            workspace.output.reset();
        }
        output.workspaces.clear();
    }
    outputs.clear();
    workspaces.clear();
    windows.clear();

    close(push_socket);
}

/******************************************************************************/
/******************************************************************************/

std::unique_ptr<char[]> I3State::message(uint32_t type)
{
    // Send query
    bool sent = send_message(push_socket, type, Application::dead);

    uint32_t ret_type = I3_INVALID_TYPE;
    std::unique_ptr<char[]> buffer;
    if(sent)
        buffer = read_message(push_socket, ret_type, Application::dead);

    if(sent && ret_type != I3_INVALID_TYPE)
        return buffer;

    return std::unique_ptr<char[]>();
}

void I3State::register_workspace(uint8_t num,
                                 string const& name,
                                 string const& output_name,
                                 bool focused)
{
    auto const output = outputs[output_name];
    // Can't use make_unique due to private constructor
    // auto ws_uptr = std::make_unique<Workspace>(json, output);
    auto ws_sptr(make_shared<Workspace>(num, name, output));
    output->workspaces[num] = ws_sptr;
    workspaces[num] = ws_sptr;

    if(focused)
    {
        focused_output = output;
        focused_output->focused_workspace = ws_sptr;
    }
}

/******************************************************************************/
/******************************************************************************/

void I3State::get_outputs(void)
{
    valid = false;

    auto buffer = message(I3_IPC_MESSAGE_TYPE_GET_OUTPUTS);
    if(buffer == nullptr)
    {
        log() << "Failed to get outputs" << endl;
        return;
    }

    char const* buffer_ptr = buffer.get();
    auto json_uptr = JSON::parse(buffer_ptr);
    auto& array = *json_uptr;

    uint8_t total_displays((uint8_t)array.size());
    log() << "Total outputs: " << (int)total_displays << endl;
    vector<OutputRect> outputs_vec;
    for(uint8_t i = 0; i < total_displays; i++)
    {
        auto& disp = array[i];
        if(disp["active"])
            outputs_vec.push_back(OutputRect(disp));
    }

    std::sort(outputs_vec.begin(), outputs_vec.end());

    log() << "Outputs" << endl;
    for(uint8_t i = 0; i < outputs_vec.size(); i++)
    {
        OutputRect& o = outputs_vec[i];
        outputs[o.name] = make_shared<Output>(i, o.name);
        log() << "    " << (int)i << ": " << o.name << endl;
    }
    valid = true;
}

/**
 * Requires valid output configuration
 */
void I3State::get_workspaces(void)
{
    valid = false;

    auto buffer = message(I3_IPC_MESSAGE_TYPE_GET_WORKSPACES);
    if(!buffer)
    {
        log() << "Failed to get workspaces" << endl;
        return;
    }

    char const* buffer_ptr = buffer.get();
    auto json_uptr = JSON::parse(buffer_ptr);
    auto& array = *json_uptr;

    for(uint8_t i = 0; i < array.size(); i++)
    {
        auto& workspace_i = array[i];
        register_workspace(workspace_i["num"],
                           workspace_i["name"],
                           workspace_i["output"],
                           workspace_i["focused"]);
    }

    valid = true;
}

void I3State::init_layout(void)
{
    get_outputs();
    if(valid)
        get_workspaces();
}

/******************************************************************************/
/******************************************************************************/

void I3State::workspace_init(JSON const& current)
{
    valid = false;

    uint8_t workspace_num(current["num"]);
    string workspace_name(current["name"]);

    auto buffer = message(I3_IPC_MESSAGE_TYPE_GET_WORKSPACES);
    if(buffer)
    {
        char const* buffer_ptr = buffer.get();
        auto json_uptr = JSON::parse(buffer_ptr);
        auto& array = *json_uptr;

        for(uint8_t i = 0; i < array.size(); i++)
        {
            auto& workspace_i = array[i];
            uint8_t num_i = workspace_i["num"];
            if(num_i == workspace_num)
            {
                string workspace_output(workspace_i["output"]);
                register_workspace(workspace_num,
                                   workspace_name,
                                   workspace_output,
                                   workspace_i["focused"]);
                valid = true;
                break;
            }
        }
    }
}

void I3State::workspace_focus(JSON const& current)
{
    valid = false;
    uint8_t workspace_num(current["num"]);

    auto const workspace(workspaces[workspace_num]);
    auto const output(workspace->output);

    focused_output = output;
    output->focused_workspace = workspace;
    valid = true;
}

void I3State::workspace_urgent(JSON const& current)
{
    valid = false;
    uint8_t num(current["num"]);
    bool urgent(current["urgent"]);

    auto const workspace = workspaces[num];
    workspace->urgent = urgent;

    valid = true;
}

void I3State::workspace_empty(JSON const& container)
{
    uint8_t workspace_num(container["num"]);
    auto const workspace = workspaces[workspace_num];

    auto const output(workspace->output);
    output->workspaces.erase(workspace_num);
    workspaces.erase(workspace_num);
}

void I3State::update_from_tree(void)
{
    valid = false;

    auto buffer = message(I3_IPC_MESSAGE_TYPE_GET_TREE);
    if(buffer != nullptr)
    {
        char const* buffer_ptr = buffer.get();
        auto json_uptr = JSON::parse(buffer_ptr);
        auto const& root = *json_uptr;

        // Root nodes are outputs.
        update_outputs_from_tree(root["nodes"]);
    }
}

void I3State::update_outputs_from_tree(JSON const& root_children)
{
    for(auto& output_obj : root_children.as_vector())
    {
        // For each output tree ...
        auto& output_name = (*output_obj)["name"]; // eDP1, __i3, LVDS0

        // ... search for output_name in list of known outputs.
        auto output_kv = outputs.find(output_name);
        // If it is an actual display ...
        if(output_kv != outputs.end())
        {
            // ... get the `Output` object (shared pointer).
            auto const output = output_kv->second;
            update_workspaces_from_tree(output, (*output_obj)["nodes"]);
        }
        // If the display configuration changed then we need to restart
        // statorange completely, because we can't adapt to sudden output
        // changes on the fly.
    }
}

void I3State::update_workspaces_from_tree(shared_ptr<Output> output,
                                          JSON const& output_children)
{
    for(auto& node : output_children.as_vector())
    {
        string const& node_type = node->get("type");

        // Outputs have different types of nodes. We only want `con`tainers.
        if(node_type.compare("con") == 0)
        {
            // Get workspaces of the current output.
            auto& workspaces_arr = (*node)["nodes"];
            for(auto& workspace_obj : workspaces_arr.as_vector())
            {
                // For each workspace tree ...
                uint8_t workspace_num = (*workspace_obj)["num"];

                // ... get the Workspace object from Output object.
                auto& workspace = workspaces[workspace_num];
                update_windows_from_tree(workspace, (*workspace_obj)["nodes"]);
            }
        }
    }
}

void I3State::update_windows_from_tree(shared_ptr<Workspace> workspace,
                                       JSON const& workspace_children)
{
    for(auto& node : workspace_children.as_vector())
    {
        if(node->get("focused"))
            workspace->focused_window_id = node->get("id");
    }
}
