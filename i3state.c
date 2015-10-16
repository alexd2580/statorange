
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>

#include"i3state.h"
#include"jsonParser.h"
#include"i3-ipc.h"
#include"i3-ipc-constants.h"
#include"strUtil.h"

/******************************************************************************/
/******************************************************************************/

void freeOutputs(I3State* i3)
{
  if(i3->outputs == NULL)
    return;
  free(i3->outputs);
  i3->outputs = NULL;
}

/** Returns 1 iff a is at a "smaller" position */
#define POS_LESS(a, b) \
  (a->posX < b->posX || (a->posX == b->posX && a->posY < b->posY))

/**
 * Returns NULL on error
 */
void parseOutputs(I3State* i3)
{
  i3->valid = 0;
  //Send query
  if(sendMessage(i3->fd, I3_IPC_MESSAGE_TYPE_GET_OUTPUTS, NULL) != 0)
    return;

  uint32_t type;
  char* buffer = readMessage(i3->fd, &type);
  if(type == I3_INVALID_TYPE)
    return;
  
  JSONArray* array = (JSONArray*)parseJSON(buffer);
  uint8_t totalDisplays = (uint8_t)getArrayLength(array);  
  uint8_t activeDisplays = 0;
  
  freeOutputs(i3);
  i3->outputs = (Output*)malloc(totalDisplays*sizeof(Output));
  
  JSONObject* disp = NULL;
  JSONBool* active = NULL;
  JSONString* name = NULL;
  JSONObject* rect = NULL;
  char* nameString;
  size_t nameLength;
  double x, y;
  for(uint8_t i=0; i<totalDisplays; i++)
  {
    disp = (JSONObject*)getElem(array, i);
    active = (JSONBool*)getField(disp, "active");
    if(getBool(active))
    {
      name = (JSONString*)getField(disp, "name");
      nameString = getString(name, &nameLength);
      strncpy(i3->outputs[activeDisplays].name, nameString, nameLength);
      i3->outputs[activeDisplays].name[nameLength] = '\0';
      //i3->outputs[activeDisplays].pos = -1; <- unused (given by ordering)
      rect = (JSONObject*)getField(disp, "rect");
      x = getNumber((JSONNumber*)getField(rect, "x"));
      y = getNumber((JSONNumber*)getField(rect, "y"));
      i3->outputs[activeDisplays].posX = (int)x;
      i3->outputs[activeDisplays].posY = (int)y;
      activeDisplays++;
    }
  }
  i3->outputCount = activeDisplays;

  Output* other = NULL;
  Output* minOut = NULL;
  Output tmp;
  int minOutPos = -1;
  for(int fPos=0; fPos<activeDisplays-1; fPos++)
  {
    minOut = i3->outputs+fPos;
    minOutPos = fPos;
    for(int oPos=fPos+1; oPos<activeDisplays; oPos++)
    {
      other = i3->outputs+oPos;
      if(POS_LESS(other, minOut))
      {
        minOut = other;
        minOutPos = oPos;
      }
    }
    
    if(minOutPos != fPos)
    {
      memcpy(&tmp, i3->outputs+fPos, sizeof(Output));
      memcpy(i3->outputs+fPos, minOut, sizeof(Output));
      memcpy(minOut, &tmp, sizeof(Output));
    }
    //minOut->pos = fPos;
  }
  
  freeJSON((JSONSomething*)array);
  free(buffer);
  
  i3->valid = 1;
  return;
}

/******************************************************************************/
/******************************************************************************/

void freeWorkspaces(I3State* i3)
{
  if(i3->workspaces == NULL)
    return;
  free(i3->workspaces);
  i3->workspaces = NULL;
}

/**
 * Returns NULL on error
 * Requires valid output configuration
 */
void parseWorkspaces(I3State* i3)
{
  i3->valid = 0;

  //Send query
  if(sendMessage(i3->fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, NULL) != 0)
    return;
    
  uint32_t type;
  char* buffer = readMessage(i3->fd, &type);
  if(type == I3_INVALID_TYPE)
    return;
  
  i3->focusedWorkspace = NULL;
  freeWorkspaces(i3);

  JSONArray* array = (JSONArray*)parseJSON(buffer);
  if(array == NULL)
    goto abort_parse_workspaces2;
  i3->wsCount = (uint8_t)getArrayLength(array);
  
  Workspace* ws = i3->workspaces = (Workspace*)malloc(i3->wsCount*sizeof(Workspace));
  Workspace* wsPtr = NULL;
  
  double numD;
  JSONObject* workspace = NULL;
  JSONString* string = NULL;
  char* strData;
  size_t strLen;

  for(uint8_t i=0; i<i3->wsCount; i++)
  {
    workspace = (JSONObject*)getElem(array, i);
    wsPtr = ws+i;
    
    numD = getNumber((JSONNumber*)getField(workspace, "num"));
    wsPtr->num = (uint8_t)numD;
    
    string = (JSONString*)getField(workspace, "name");
    strData = getString(string, &strLen);
    storeString(wsPtr->name, MAX_NAME_LEN, strData, strLen);
    
    wsPtr->visible = getBool((JSONBool*)getField(workspace, "visible"));
    wsPtr->focused = getBool((JSONBool*)getField(workspace, "focused"));
    if(wsPtr->focused) i3->focusedWorkspace = wsPtr;
    wsPtr->urgent = getBool((JSONBool*)getField(workspace, "urgent"));
    
    string = (JSONString*)getField(workspace, "output");
    strData = getString(string, &strLen);
    
    for(uint8_t d=0; d<i3->outputCount; d++)
      if(strncmp(i3->outputs[d].name, strData, strLen) == 0)
        wsPtr->output = i3->outputs+d;
        
    wsPtr->focusedApp[0] = '\0';
    wsPtr->focusedAppID = -1;
  }

  i3->valid = 1;

  freeJSON((JSONSomething*)array);
abort_parse_workspaces2:
  free(buffer);
  
  return;
}

/******************************************************************************/
/******************************************************************************/

void init_i3State(I3State* i3, char* path)
{
  pthread_mutex_init(&i3->mutex, NULL);
  pthread_mutex_lock(&i3->mutex);
  
  i3->fd = init_socket(path);
  memset(i3->mode, 0, 50);
  i3->outputCount = 0;
  i3->outputs = NULL;
  i3->wsCount = 0;
  i3->workspaces = NULL;
  i3->focusedWorkspace = NULL;
  
  pthread_mutex_unlock(&i3->mutex);
}

void update_i3StateOutputs(I3State* i3)
{
  pthread_mutex_lock(&i3->mutex);
  parseOutputs(i3);
  if(i3->valid)
    parseWorkspaces(i3);
  pthread_mutex_unlock(&i3->mutex);
}

void update_i3StateWorkspaceInit(I3State* i3, uint8_t num)
{
  pthread_mutex_lock(&i3->mutex);
  i3->valid = 0;

  //Send query
  if(sendMessage(i3->fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, NULL) != 0)
    goto abort_init_update3;
    
  uint32_t type;
  char* buffer = readMessage(i3->fd, &type);
  if(type == I3_INVALID_TYPE)
    goto abort_init_update3;
  
  JSONArray* array = (JSONArray*)parseJSON(buffer);
  if(array == NULL)
    goto abort_init_update2;
  
  Workspace* oldWsArray = i3->workspaces;
  Workspace* newWsArray = (Workspace*)malloc((uint8_t)(i3->wsCount+1)*sizeof(Workspace));
  uint8_t wsBefore = 0;
  while(oldWsArray[wsBefore].num < num && wsBefore < i3->wsCount)
    wsBefore++;
  
  memmove(newWsArray, oldWsArray, wsBefore*sizeof(Workspace));
  memmove(newWsArray+wsBefore+1, oldWsArray+wsBefore, (uint8_t)(i3->wsCount-wsBefore)*sizeof(Workspace));
  
  i3->focusedWorkspace += newWsArray - oldWsArray;
  if(i3->focusedWorkspace == newWsArray+wsBefore)
    i3->focusedWorkspace++;
  free(i3->workspaces);
  i3->workspaces = newWsArray;
  
  Workspace* ws = i3->workspaces + wsBefore;
  ws->num = num;
  i3->wsCount++;
  
  double numD;
  JSONObject* wsJSON = NULL;

  for(uint8_t i=0; i<i3->wsCount; i++)
  {
    wsJSON = (JSONObject*)getElem(array, i);
    
    numD = getNumber((JSONNumber*)getField(wsJSON, "num"));
    if((uint8_t)numD == num)
      break;
  }
  if(wsJSON == NULL)
    goto abort_init_update1;
  
  JSONString* string = NULL;
  char* strData;
  size_t strLen;
  string = (JSONString*)getField(wsJSON, "name");
  strData = getString(string, &strLen);
  storeString(ws->name, MAX_NAME_LEN, strData, strLen);

  ws->visible = getBool((JSONBool*)getField(wsJSON, "visible"));
  ws->focused = getBool((JSONBool*)getField(wsJSON, "focused"));
  if(ws->focused) i3->focusedWorkspace = ws;
  ws->urgent = getBool((JSONBool*)getField(wsJSON, "urgent"));
  
  string = (JSONString*)getField(wsJSON, "output");
  strData = getString(string, &strLen);
  
  for(uint8_t d=0; d<i3->outputCount; d++)
    if(strncmp(i3->outputs[d].name, strData, strLen) == 0)
      ws->output = i3->outputs+d;
      
  ws->focusedApp[0] = '\0';
  ws->focusedAppID = -1;
  
  i3->valid = 1;

abort_init_update1:
  freeJSON((JSONSomething*)array);
abort_init_update2:
  free(buffer);
abort_init_update3:
  pthread_mutex_unlock(&i3->mutex);

  return;
}

void update_i3StateWorkspaceStatus(I3State* i3)
{
  pthread_mutex_lock(&i3->mutex);
  i3->valid = 0;

  //Send query
  if(sendMessage(i3->fd, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, NULL) != 0)
    goto abort_status_update3;
    
  uint32_t type;
  char* buffer = readMessage(i3->fd, &type);
  if(type == I3_INVALID_TYPE)
    goto abort_status_update3;
  
  i3->focusedWorkspace = NULL;
  JSONArray* array = (JSONArray*)parseJSON(buffer);
  if(array == NULL)
    goto abort_status_update2;
  int arrayLen = getArrayLength(array);
  
  double numD;
  int numI;
  JSONObject* workspace = NULL;
  Workspace* ws;

  for(int i=0; i<arrayLen; i++)
  {
    workspace = (JSONObject*)getElem(array, i);
    
    numD = getNumber((JSONNumber*)getField(workspace, "num"));
    numI = (int)numD;
    
    ws = NULL;
    for(int j=0; j<i3->wsCount; j++)
      if(i3->workspaces[j].num == numI)
      {
        ws = i3->workspaces+j;
        break;
      }
    if(ws == NULL)
      goto abort_status_update1;
    
    ws->visible = getBool((JSONBool*)getField(workspace, "visible"));
    ws->focused = getBool((JSONBool*)getField(workspace, "focused"));
    if(ws->focused) i3->focusedWorkspace = ws;
    ws->urgent = getBool((JSONBool*)getField(workspace, "urgent"));
  }
  
  i3->valid = 1;

abort_status_update1:
  freeJSON((JSONSomething*)array);
abort_status_update2:
  free(buffer);
abort_status_update3:
  pthread_mutex_unlock(&i3->mutex);

  return;
}

void update_i3StateWorkspaceEmpty(I3State* i3, uint8_t num)
{
  pthread_mutex_lock(&i3->mutex);
  i3->valid = 0;

  
  uint8_t wsBefore = 0;
  while(i3->workspaces[wsBefore].num != num && wsBefore < i3->wsCount)
    wsBefore++;
  
  if(wsBefore == i3->wsCount)
    goto abort_empty_update;
    
  i3->wsCount--;
  Workspace* ws = i3->workspaces+wsBefore;
  memmove(ws, ws+1, (uint8_t)(i3->wsCount-wsBefore)*sizeof(Workspace));

  if(i3->focusedWorkspace > ws)
    i3->focusedWorkspace--;
  
  i3->valid = 1;

abort_empty_update:
  pthread_mutex_unlock(&i3->mutex);

  return;
}

void free_i3State(I3State* i3)
{
  pthread_mutex_lock(&i3->mutex);
  
  freeWorkspaces(i3);
  freeOutputs(i3);
  shutdown(i3->fd, SHUT_RDWR);
  
  pthread_mutex_unlock(&i3->mutex);
  pthread_mutex_destroy(&i3->mutex);
}


