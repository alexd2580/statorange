
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

Workspace::Workspace(JSON const& json, Output& parent)
    : num(json["num"]), name(json["name"]), output(parent)
{
  visible = json["visible"];
  focused = json["focused"];
  urgent = json["urgent"];

  focused_window_id = -1;
  focused_window_name = "";
}

void Workspace::register_workspace(JSON const& json,
                                   map<string, Output>& outputs)
{
  string output_name(json["output"]);
  Output& output = outputs[output_name];
  // Can't use make_unique due to private constructor
  // auto ws_uptr = std::make_unique<Workspace>(json, output);
  std::unique_ptr<Workspace> ws_uptr(new Workspace(json, output));
  output.workspaces[ws_uptr->num] = std::move(ws_uptr);
}

/******************************************************************************/
/******************************************************************************/

void I3State::get_outputs(void)
{
  valid = false;
  // Send query
  if(!send_message(fd, I3_IPC_MESSAGE_TYPE_GET_OUTPUTS, die))
    return;

  uint32_t type;
  auto buffer = read_message(fd, &type, die);
  if(type == I3_INVALID_TYPE)
    return;

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
      outputs[o.name] = {i, o.name, decltype(Output::workspaces)()};
      log() << "\t" << (int)i << ": " << o.name << endl;
    }
  }
  catch(JSONException& e)
  {
    e.printStackTrace();
  }

  valid = true;
}

/******************************************************************************/
/******************************************************************************/

/**
 * Requires valid output configuration
 * Resets window focus information
 */
void I3State::get_workspaces(void)
{
  valid = false;

  // Send query
  if(!send_message(fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, die))
    return;

  uint32_t type;
  auto buffer = read_message(fd, &type, die);
  if(type == I3_INVALID_TYPE)
    return;

  try
  {
    auto json_uptr = JSON::parse(buffer.get());
    auto& array = *json_uptr;
    size_t ws_count = array.size();

    for(uint8_t i = 0; i < ws_count; i++)
      Workspace::register_workspace(array[i], outputs);

    valid = true;
  }
  catch(JSONException& e)
  {
    e.printStackTrace();
  }
}

/******************************************************************************/
/******************************************************************************/

I3State::I3State(string& path, bool& die_)
    : Logger("[I3State]", cerr), fd(init_socket(path)), die(die_)
{
  mutex.lock();
  mode = "default";
  valid = false;
  mutex.unlock();
}

I3State::~I3State()
{
  mutex.lock();
  close(fd);
  mutex.unlock();
}

void I3State::init_layout(void)
{
  mutex.lock();
  get_outputs();
  if(valid)
    get_workspaces();
  mutex.unlock();
}

void I3State::workspace_init(uint8_t num)
{
  mutex.lock();
  valid = false;

  // Send query
  bool sent = send_message(fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, die);

  uint32_t type = I3_INVALID_TYPE;
  std::unique_ptr<char[]> buffer;
  if(sent)
    buffer = read_message(fd, &type, die);

  if(sent && type != I3_INVALID_TYPE)
  {
    try
    {
      auto json_uptr = JSON::parse(buffer.get());
      auto& array = *json_uptr;

      for(uint8_t i = 0; i < array.size(); i++)
      {
        auto& json_ws = array[i];
        uint8_t anum = json_ws["num"];
        if(anum == num)
        {
          Workspace::register_workspace(json_ws, outputs);
          valid = true;
          break;
        }
      }
    }
    catch(TraceCeption& e)
    {
      e.printStackTrace();
    }
  }
  mutex.unlock();
}

struct ThreeBools
{
  bool visible;
  bool focused;
  bool urgent;
};

void I3State::workspace_status(void)
{
  mutex.lock();
  valid = false;

  // Send query
  bool sent = send_message(fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, die);

  uint32_t type = I3_INVALID_TYPE;
  std::unique_ptr<char[]> buffer;
  if(sent)
    buffer = read_message(fd, &type, die);

  if(sent && type != I3_INVALID_TYPE)
  {
    try
    {
      auto json_uptr = JSON::parse(buffer.get());
      auto& array = *json_uptr;
      size_t arrayLen = array.size();

      std::map<uint8_t, ThreeBools> workspace_states;

      for(size_t i = 0; i < arrayLen; i++)
      {
        auto& workspace = array[i];
        uint8_t num = workspace["num"];
        workspace_states[num] = {
            workspace["visible"], workspace["focused"], workspace["urgent"]};
      }

      for(auto& output_pair : outputs)
      {
        auto& output = output_pair.second;
        for(auto& ws : output.workspaces)
        {
          uint8_t num = ws.first;
          auto& ws_obj = ws.second;
          auto& ws_state = workspace_states[num];
          ws_obj->visible = ws_state.visible;
          ws_obj->focused = ws_state.focused;
          ws_obj->urgent = ws_state.urgent;
        }
      }

      valid = true;
    }
    catch(JSONException& e)
    {
      e.printStackTrace();
    }
  }
  mutex.unlock();
}

void I3State::workspace_empty(uint8_t num)
{
  mutex.lock();

  for(auto& output_pair : outputs)
  {
    auto& output = output_pair.second;
    auto entry = output.workspaces.find(num);
    if(entry != output.workspaces.end())
    {
      output.workspaces.erase(entry);
      break;
    }
  }

  mutex.unlock();
}
