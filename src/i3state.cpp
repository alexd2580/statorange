
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>

#include <sys/socket.h>

#include "JSON/jsonParser.hpp"

#include "i3state.hpp"
#include "i3-ipc.hpp"
#include "i3-ipc-constants.hpp"
#include "util.hpp"

using namespace std;

/******************************************************************************/
/******************************************************************************/

struct Output
{
  std::string name;
  int x;
  int y;
};

/** Returns true iff a is at a "smaller" position */
__attribute__((pure)) bool sort_by_pos(Output const& a, Output const& b)
{
  return a.x < b.x || (a.x == b.x && a.y < b.y);
}

void I3State::parseOutputs(void)
{
  valid = false;
  // Send query
  if(sendMessage(fd, I3_IPC_MESSAGE_TYPE_GET_OUTPUTS, nullptr) != 0)
    return;

  uint32_t type;
  char* buffer = readMessage(fd, &type);
  if(type == I3_INVALID_TYPE)
    return;

  try
  {
    JSON* json = JSON::parse(buffer);
    JSONArray& array = json->array();
    uint8_t total_displays = (uint8_t)array.size();

    vector<Output> outputs_vec;
    for(uint8_t i = 1; i < total_displays; i++)
    {
      JSONObject& disp = array[i].object();
      bool active = disp["active"].boolean();
      if(active)
      {
        string name = disp["name"].string();
        JSONObject& rect = disp["rect"].object();
        int x = rect["x"].number();
        int y = rect["y"].number();

        outputs_vec.push_back({name, x, y});
      }
    }
    delete json;

    std::sort(outputs_vec.begin(), outputs_vec.end(), sort_by_pos);

    for(unsigned int i = 0; i < outputs.size(); i++)
    {
      Output& o = outputs_vec[i];
      outputs[o.name] = i;
    }
  }
  catch(JSONException& e)
  {
    e.printStackTrace();
  }

  free(buffer);
  valid = true;
  return;
}

/******************************************************************************/
/******************************************************************************/

/**
 * Returns NULL on error
 * Requires valid output configuration
 */
void I3State::parseWorkspaces(void)
{
  valid = false;

  // Send query
  if(sendMessage(fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, nullptr) != 0)
    return;

  uint32_t type;
  char* buffer = readMessage(fd, &type);
  if(type == I3_INVALID_TYPE)
    return;

  focusedWorkspace = 0;
  workspaces.clear();

  try
  {
    JSON* json = JSON::parse(buffer);
    JSONArray& array = json->array();
    size_t wsCount = array.size();

    for(uint8_t i = 0; i < wsCount; i++)
    {
      Workspace ws;
      JSONObject& workspace = array[i].object();

      ws.num = workspace["num"].number();
      ws.name = workspace["name"].string(); // TODO LEN
      ws.visible = workspace["visible"].boolean();
      ws.focused = workspace["focused"].boolean();
      ws.urgent = workspace["urgent"].boolean();

      string output = workspace["output"].string();
      ws.output = outputs[output];

      ws.focusedApp = "";
      ws.focusedAppID = -1;

      workspaces.push_back(ws);
      if(ws.focused)
        focusedWorkspace = workspaces.size() - 1;
    }

    delete json;
    valid = true;
  }
  catch(JSONException& e)
  {
    e.printStackTrace();
  }
  return;
}

/******************************************************************************/
/******************************************************************************/

I3State::I3State(string path) : fd(init_socket(path.c_str()))
{
  pthread_mutex_init(&mutex, nullptr);
  pthread_mutex_lock(&mutex);

  focusedWorkspace = 0;
  mode = "default";
  valid = false;

  pthread_mutex_unlock(&mutex);
}

I3State::~I3State()
{
  pthread_mutex_lock(&mutex);

  shutdown(fd, SHUT_RDWR);

  pthread_mutex_unlock(&mutex);
  pthread_mutex_destroy(&mutex);
}

void I3State::updateOutputs(void)
{
  pthread_mutex_lock(&mutex);
  parseOutputs();
  if(valid)
    parseWorkspaces();
  pthread_mutex_unlock(&mutex);
}

void I3State::workspaceInit(uint8_t num)
{
  pthread_mutex_lock(&mutex);
  valid = false;

  // Send query
  bool sent = sendMessage(fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, nullptr) == 0;

  uint32_t type;
  char* buffer = nullptr;
  if(sent)
    buffer = readMessage(fd, &type);

  if(sent && type != I3_INVALID_TYPE)
  {
    try
    {
      JSON* json = JSON::parse(buffer);
      JSONArray& array = json->array();

      uint8_t index = 0;
      auto iter = workspaces.begin();
      while(iter->num < num && iter != workspaces.end())
      {
        iter++;
        index++;
      }

      if(workspaces[focusedWorkspace].num > num)
        focusedWorkspace++;

      for(uint8_t i = 0; i < array.size(); i++)
      {
        JSONObject& wsJSON = array[i].object();

        uint8_t anum = wsJSON["num"].number();
        if(anum == num)
        {
          Workspace ws;
          ws.num = num;
          ws.name = wsJSON["name"].string(); // TODO length
          ws.visible = wsJSON["visible"].boolean();
          ws.focused = wsJSON["focused"].boolean();
          ws.urgent = wsJSON["urgent"].boolean();

          ws.focusedApp = "";
          ws.focusedAppID = -1;

          string output = wsJSON["output"].string();
          ws.output = outputs[output];

          workspaces.insert(iter, ws);
          if(ws.focused)
            focusedWorkspace = index;

          valid = true;
          break;
        }
      }
      delete json;
    }
    catch(TraceCeption& e)
    {
      e.printStackTrace();
    }
    free(buffer);
  }
  pthread_mutex_unlock(&mutex);
}

void I3State::updateWorkspaceStatus(void)
{
  pthread_mutex_lock(&mutex);
  valid = false;

  // Send query
  bool sent = sendMessage(fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, nullptr) == 0;

  uint32_t type;
  char* buffer = nullptr;
  if(sent)
    buffer = readMessage(fd, &type);

  if(sent && type != I3_INVALID_TYPE)
  {
    focusedWorkspace = 0;
    try
    {
      JSON* json = JSON::parse(buffer);
      JSONArray& array = json->array();

      size_t arrayLen = array.size();

      for(size_t i = 0; i < arrayLen; i++)
      {
        JSONObject& workspace = array[i].object();
        uint8_t num = workspace["num"].number();

        for(size_t j = 0; j < workspaces.size(); j++)
        {
          Workspace& ws = workspaces[j];
          if(ws.num == num)
          {
            ws.visible = workspace["visible"].boolean();
            ws.focused = workspace["focused"].boolean();
            ws.urgent = workspace["urgent"].boolean();
            if(ws.focused)
              focusedWorkspace = j;
            break;
          }
        }
      }

      valid = true;

      delete json;
    }
    catch(JSONException& e)
    {
      e.printStackTrace();
    }
    free(buffer);
  }
  pthread_mutex_unlock(&mutex);
}

void I3State::workspaceEmpty(uint8_t num)
{
  pthread_mutex_lock(&mutex);
  valid = false;

  uint8_t wsBefore = 0;
  while(workspaces[wsBefore].num != num && wsBefore < workspaces.size())
    wsBefore++;

  if(wsBefore < workspaces.size())
  {
    workspaces.erase(workspaces.begin() + wsBefore);
    if(focusedWorkspace > wsBefore)
      focusedWorkspace--;

    valid = true;
  }
  pthread_mutex_unlock(&mutex);
}
