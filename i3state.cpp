
#include<cstdlib>
#include<cstring>

#include<sys/socket.h>

#include"i3state.hpp"
#include"jsonParser.hpp"
#include"i3-ipc.hpp"
#include"i3-ipc-constants.hpp"
#include"strUtil.hpp"

/******************************************************************************/
/******************************************************************************/

/** Returns 1 iff a is at a "smaller" position */
#define POS_LESS(a, b) \
  (a.posX < b.posX || (a.posX == b.posX && a.posY < b.posY))

/**
 * Returns NULL on error
 */
void I3State::parseOutputs(void)
{
  valid = false;
  //Send query
  if(sendMessage(fd, I3_IPC_MESSAGE_TYPE_GET_OUTPUTS, nullptr) != 0)
    return;

  uint32_t type;
  char* buffer = readMessage(fd, &type);
  if(type == I3_INVALID_TYPE)
    return;
  
  JSONArray& array = JSON::parse(buffer).array();
  uint8_t totalDisplays = (uint8_t)array.length();  
  
  outputs.clear();
  
  double x, y;
  for(uint8_t i=0; i<totalDisplays; i++)
  {
    JSONObject& disp = array.get(i).object();
    bool active = disp.get("active").boolean().get();
    if(active)
    {
      string name = disp.get("name").string().get();
      JSONObject& rect = disp.get("rect").object();
      x = rect.get("x").number().get();
      y = rect.get("y").number().get();

      outputs.push_back(Output { .name=name, .posX=(int)x, .posY=(int)y } );
    }
  }

  
  Output tmp;
  size_t minOutPos;
  
  for(size_t fPos=0; fPos<outputs.size()-1; fPos++)
  {
    minOutPos = fPos;
    for(size_t oPos=fPos+1; oPos<outputs.size(); oPos++)
    {
      if(POS_LESS(outputs[oPos], outputs[fPos]))
        minOutPos = oPos;
    }
    
    if(minOutPos != fPos)
    {
      tmp = outputs[fPos];
      outputs[fPos] = outputs[minOutPos];
      outputs[minOutPos] = tmp;
    }
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

  //Send query
  if(sendMessage(fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, nullptr) != 0)
    return;
    
  uint32_t type;
  char* buffer = readMessage(fd, &type);
  if(type == I3_INVALID_TYPE)
    return;
  
  focusedWorkspace = 0;
  workspaces.clear();

  JSONArray& array = JSON::parse(buffer).array();
  size_t wsCount = array.length();
  
  double numD;

  for(uint8_t i=0; i<wsCount; i++)
  {
    Workspace ws;
    JSONObject& workspace = array.get(i).object();
    
    numD = workspace.get("num").number().get();
    ws.num = (uint8_t)numD;
    
    JSONString& name = workspace.get("name").string();
    ws.name = name.get(); // TODO LEN
    
    ws.visible = workspace.get("visible").boolean().get();
    ws.focused = workspace.get("focused").boolean().get();
    ws.urgent = workspace.get("urgent").boolean().get();
    
    string output = workspace.get("output").string().get();
    for(uint8_t d=0; d<outputs.size(); d++)
      if(compare(outputs[d].name, output) == 0)
        ws.output = &outputs[d];
        
    ws.focusedApp = "";
    ws.focusedAppID = -1;
    
    workspaces.push_back(ws);
    if(ws.focused) 
      focusedWorkspace = workspaces.size()-1;
  }

  valid = true;
  return;
}

/******************************************************************************/
/******************************************************************************/

I3State::I3State(string path)
{
  pthread_mutex_init(&mutex, nullptr);
  pthread_mutex_lock(&mutex);
  
  fd = init_socket(path);
  outputCount = 0;
  outputs = nullptr;
  wsCount = 0;
  workspaces = nullptr;
  focusedWorkspace = nullptr;
  
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

  //Send query
  if(sendMessage(fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, nullptr) != 0)
    goto abort_init_update3;
    
  uint32_t type;
  char* buffer = readMessage(fd, &type);
  if(type == I3_INVALID_TYPE)
    goto abort_init_update3;
  
  JSONArray& array = JSON::parse(buffer).array();
    goto abort_init_update2; //TODO
  
  uint8_t index = 0;
  auto iter = workspaces.begin();
  while(iter->num < num && iter != workspaces.end())
  {
    iter++;
    index++;
  }
    
  if(workspaces[focusedWorkspace].num > num)
    focusedWorkspace++;
  
  for(uint8_t i=0; i<wsCount; i++)
  {
    JSONObject& wsJSON = array.get(i).object();
    
    double numD = wsJSON.get("num").number().get();
    if((uint8_t)numD == num)
    {
      Workspace ws;
      ws.num = num;
      
      JSONString& name = wsJSON.get("name").string();
      ws.name = name.get(); // TODO length
      
      ws.visible = wsJSON.get("visible").boolean().get();
      ws.focused = wsJSON.get("focused").boolean().get();
      ws.urgent = wsJSON.get("urgent").boolean().get();
      
      ws.focusedApp = "";
      ws.focusedAppID = -1;
  
      string output = wsJSON.get("output").string().get();
      for(size_t d=0; d<outputs.size(); d++)
        if(compare(outputs[d].name, output) == 0)
          ws.output = &outputs[d];
      
      workspaces.insert(iter, ws);
      if(ws->focused) 
        focusedWorkspace = index;
        
      valid = true;
      break;
    }
  }
  
  free(buffer);
  pthread_mutex_unlock(&mutex);
}

void I3State::updateWorkspaceStatus(void)
{
  pthread_mutex_lock(&mutex);
  valid = false;

  //Send query
  if(sendMessage(fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, nullptr) != 0)
    goto abort_status_update3;
    
  uint32_t type;
  char* buffer = readMessage(fd, &type);
  if(type == I3_INVALID_TYPE)
    goto abort_status_update3;
  
  focusedWorkspace = nullptr;
  JSONArray* array = (JSONArray*)parseJSON(buffer);
  if(array == nullptr)
    goto abort_status_update2;
  int arrayLen = getArrayLength(array);
  
  double numD;
  int numI;
  JSONObject* workspace = nullptr;
  Workspace* ws;

  for(int i=0; i<arrayLen; i++)
  {
    workspace = (JSONObject*)getElem(array, i);
    
    numD = getNumber((JSONNumber*)getField(workspace, "num"));
    numI = (int)numD;
    
    ws = nullptr;
    for(int j=0; j<wsCount; j++)
      if(workspaces[j].num == numI)
      {
        ws = workspaces+j;
        break;
      }
    if(ws == nullptr)
      goto abort_status_update1;
    
    ws->visible = getBool((JSONBool*)getField(workspace, "visible"));
    ws->focused = getBool((JSONBool*)getField(workspace, "focused"));
    if(ws->focused) focusedWorkspace = ws;
    ws->urgent = getBool((JSONBool*)getField(workspace, "urgent"));
  }
  
  valid = false;

abort_status_update1:
  freeJSON((JSONSomething*)array);
abort_status_update2:
  free(buffer);
abort_status_update3:
  pthread_mutex_unlock(&mutex);
}

void I3State::workspaceEmpty(uint8_t num)
{
  pthread_mutex_lock(&mutex);
  valid = false;

  uint8_t wsBefore = 0;
  while(workspaces[wsBefore].num != num && wsBefore < workspaces.size())
    wsBefore++;
  
  if(wsBefore == wsCount)
  {
    pthread_mutex_unlock(&mutex);
    return;
  }
  
  workspaces.erase(workspaces.begin()+wsBefore);
  if(focusedWorkspace > wsBefore)
    focusedWorkspace--;
    
  valid = true;
  pthread_mutex_unlock(&mutex);
}


