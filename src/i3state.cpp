
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <sys/socket.h>

#include "JSON/jsonParser.hpp"
#include "JSON/JSONException.hpp"

#include "i3-ipc-constants.hpp"
#include "i3-ipc.hpp"
#include "i3state.hpp"
#include "util.hpp"

using namespace std;

/******************************************************************************/
/******************************************************************************/

struct Output_rect
{
  std::string name;
  int x;
  int y;

  static Output_rect from_json(JSON const& output)
  {
    string name(output["name"]);
    auto& rect = output["rect"];
    int x = rect["x"];
    int y = rect["y"];

    return {name, x, y};
  }
};

/** Returns true iff a is at a "smaller" position */
__attribute__((pure)) bool sort_by_pos(Output_rect const& a,
                                       Output_rect const& b)
{
  return a.x < b.x || (a.x == b.x && a.y < b.y);
}

/******************************************************************************/
/******************************************************************************/

Output::Output(uint8_t pos, std::string const& name_)
{
  position = pos;
  name = name_;
}

void Output::update_from_tree(const JSON& json)
{
  // get nodes and search for "con"
  auto& output_nodes = json["nodes"];
  for(uint8_t i = 0; i < output_nodes.size(); i++)
  {
    auto& node = output_nodes[i];
    string& node_type = node["type"];
    if(node_type.compare("con") == 0)
    {
      // get workspaces of the current output
      auto& workspaces_arr = node["nodes"];
      for(uint8_t j = 0; j < workspaces_arr.size(); j++)
      {
        // for each workspace tree
        auto& workspace_obj = workspaces_arr[j];
        uint8_t workspace_num = workspace_obj["num"];

        // get Workspace object from Output object
        auto& workspace = workspaces[workspace_num];
        workspace->update_from_tree(workspace_obj);
      }
    }
  }
}

/******************************************************************************/
/******************************************************************************/

Workspace::Workspace(uint8_t num_,
                     string const& name_,
                     std::shared_ptr<Output> parent)
    : num(num_), name(name_), output(parent)
{
  urgent = false;
  focused_window_id = -1;
}

void Workspace::update_windows_from_tree(JSON const& json)
{
  for(size_t i = 0; i < json.size(); i++)
  {
    auto& node = json[i];
    if(node["focused"])
      focused_window_id = node["id"];
    update_windows_from_tree(node["nodes"]);
  }
}

void Workspace::update_from_tree(JSON const& json)
{
  update_windows_from_tree(json["nodes"]);
}

/******************************************************************************/
/******************************************************************************/

I3State::I3State(string& path, bool& die_)
    : Logger("[I3State]"), fd(init_socket(path)), die(die_)
{
  unique_lock<std::mutex> lock(this->mutex);
  mode = "default";
  valid = false;
}

I3State::~I3State(void)
{
  unique_lock<std::mutex> lock(this->mutex);

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

  close(fd);
}

/******************************************************************************/
/******************************************************************************/

std::unique_ptr<char[]> I3State::message(uint32_t type)
{
  // Send query
  bool sent = send_message(fd, type, die);

  uint32_t ret_type = I3_INVALID_TYPE;
  std::unique_ptr<char[]> buffer;
  if(sent)
    buffer = read_message(fd, &ret_type, die);

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
  if(!buffer)
  {
    log() << "Failed to get outputs" << endl;
    return;
  }

  try
  {
    auto json_uptr = JSON::parse(buffer.get());
    auto& array = *json_uptr;
    uint8_t total_displays((uint8_t)array.size());
    log() << "Total outputs: " << (int)total_displays << endl;
    vector<Output_rect> outputs_vec;
    for(uint8_t i = 0; i < total_displays; i++)
    {
      auto& disp = array[i];
      if(disp["active"])
        outputs_vec.push_back(Output_rect::from_json(disp));
    }

    std::sort(outputs_vec.begin(), outputs_vec.end(), sort_by_pos);

    log() << "Outputs" << endl;
    for(uint8_t i = 0; i < outputs_vec.size(); i++)
    {
      Output_rect& o = outputs_vec[i];
      outputs[o.name] = make_shared<Output>(i, o.name);
      log() << "\t" << (int)i << ": " << o.name << endl;
    }
    valid = true;
  }
  catch(JSONException& e)
  {
    log() << "Caught exception in I3State::get_outputs:" << endl;
    e.printStackTrace();
  }
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

  try
  {
    auto json_uptr = JSON::parse(buffer.get());
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
  catch(JSONException& e)
  {
    log() << "Caught exception in I3State::get_workspaces:" << endl;
    e.printStackTrace();
  }
}

void I3State::init_layout(void)
{
  unique_lock<std::mutex> lock(this->mutex);
  get_outputs();
  if(valid)
    get_workspaces();
}

/******************************************************************************/
/******************************************************************************/

void I3State::workspace_init(JSON const& current)
{
  unique_lock<std::mutex> lock(this->mutex);
  valid = false;

  uint8_t workspace_num(current["num"]);
  string workspace_name(current["name"]);

  auto buffer = message(I3_IPC_MESSAGE_TYPE_GET_WORKSPACES);
  if(buffer)
  {
    auto json_uptr = JSON::parse(buffer.get());
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
  unique_lock<std::mutex> lock(this->mutex);
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
  unique_lock<std::mutex> lock(this->mutex);
  valid = false;
  uint8_t num(current["num"]);
  bool urgent(current["urgent"]);

  auto const workspace = workspaces[num];
  workspace->urgent = urgent;

  valid = true;
}

void I3State::workspace_empty(JSON const& container)
{
  unique_lock<std::mutex> lock(this->mutex);
  uint8_t workspace_num(container["num"]);
  auto const workspace = workspaces[workspace_num];

  auto const output(workspace->output);
  output->workspaces.erase(workspace_num);
  workspaces.erase(workspace_num);
}

// template <typename T> class TD;

void I3State::update_from_tree(void)
{
  unique_lock<std::mutex> lock(this->mutex);
  valid = false;

  auto buffer = message(I3_IPC_MESSAGE_TYPE_GET_TREE);
  if(buffer)
  {
    try
    {
      auto json_uptr = JSON::parse(buffer.get());
      // root object
      auto const& root = *json_uptr;
      // root nddes are outputs
      auto const& root_nodes = root["nodes"];

      for(uint8_t i = 0; i < root_nodes.size(); i++)
      {
        // for each output tree
        auto const& output_obj = root_nodes[i];
        auto& output_name = output_obj["name"]; // eDP1, __i3, LVDS0

        // search for output_name in list of known outputs
        auto output_kv = outputs.find(output_name);
        // if it is an actual display
        if(output_kv != outputs.end())
        {
          // get the Output object
          auto const output = output_kv->second;
          output->update_from_tree(output_obj);
        }
      }
    }
    catch(TraceCeption& e)
    {
      e.printStackTrace();
    }
  }
}
