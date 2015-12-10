#include<csignal>
#include<iostream>
#include<cstring>
#include<pthread.h>
#include<sys/socket.h>

#include"JSON/jsonParser.hpp"
#include"JSON/jsonSearch.hpp"

#include"i3state.hpp"
#include"i3-ipc-constants.hpp"
#include"i3-ipc.hpp"
#include"util.hpp"

using namespace std;

/******************************************************************************/

extern pthread_t event_listener_thread;
extern pthread_cond_t notifier;

extern volatile sig_atomic_t die;
extern volatile sig_atomic_t exit_status;
extern volatile sig_atomic_t force_update;

Logger evlog("[EventHandler]", cerr);

#define GET_DISPLAY_NUM \
  ptr = getJSONObjectField(response, "current", 7); \
  ptr = getJSONObjectField(ptr, "num", 3); \
  double n; \
  getJSONNumber(ptr, &n);

void handleWorkspaceEvent(I3State& i3State, char* response)
{
  char const* ptr = getJSONObjectField(response, "change", 6);
  size_t evTypeLen;
  char const* eventType;
  ptr = getJSONString(ptr, &eventType, &evTypeLen);
  
  if(evTypeLen == 4 && strncmp(eventType, "init", 4) == 0)
  {
    GET_DISPLAY_NUM
    i3State.workspaceInit((uint8_t)n);
  }
  else if(evTypeLen == 5 && strncmp(eventType, "focus", 5) == 0)
    i3State.updateWorkspaceStatus();
  else if(evTypeLen == 6 && strncmp(eventType, "urgent", 6) == 0)
    i3State.updateWorkspaceStatus();
  else if(evTypeLen == 5 && strncmp(eventType, "empty", 5) == 0)
  {
    GET_DISPLAY_NUM
    i3State.workspaceEmpty((uint8_t)n);
  }
  else
  {
    evlog.log() << "Unhandled workspace event type: ";
    evlog.log().write(eventType, (int)evTypeLen);
    evlog.log() << endl;
  }
  return;
}

string getWindowName(JSONObject& container)
{
  string name = container["name"].string();
  if(name.length() > 20)
  {
    string class_ = container["window_properties"].object()["class"].string();
    if(class_.length() > 20)
    {
      string sub = name.substr(0, 15);
      return sub + "[...]";
    }
    else
      return class_;
  }
  else
    return name;
}

void handleWindowEvent(I3State& i3State, char* response)
{
  JSON* json = JSON::parse(response);
  JSONObject& windowEvent = json->object();
  string change = windowEvent["change"].string();
  
  JSONObject& container = windowEvent["container"].object();
  long appId = container["id"].number();
  
  /** New events are also accompanied by focus events if necessary */
  //cLen == 3 && strncmp(cStr, "new", 3) == 0
  
  /**
   * On a title update check if the application is focused 
   * on any visible workspace. If yes -> update (id does not change)
   */  
  if(change.compare("title") == 0)
  {
    for(auto i=i3State.workspaces.begin(); i!=i3State.workspaces.end(); i++)
      if(i->focusedAppID == appId)
      {
        i->focusedApp = getWindowName(container);
        break;
      }
  }
  /** Copy the title/id of the currently focused window to it's WS. */  
  else if(change.compare("focus") == 0)
  {
    Workspace& fw = i3State.workspaces[i3State.focusedWorkspace];
    fw.focusedApp = getWindowName(container);
    fw.focusedAppID = appId;
  }
  /** 
   * usually after a close event a focus event is issued, 
   * if there is a focused window. therefore delete the focus here.
   * I don't know on which workspace the application closed.
   */
  else if(change.compare("close") == 0)
  {
    for(auto i=i3State.workspaces.begin(); i!=i3State.workspaces.end(); i++)
    {
      if(i->focusedAppID == appId)
      {
        i->focusedAppID = -1;
        i->focusedApp = "";
        break;
      }
    }
  }
  else if(change.compare("fullscreen_mode") == 0)
  {
    evlog.log() << "Fullscreen mode TODO if anything at all..." << endl;
  }
  else if(change.compare("move") == 0)
  {
    evlog.log() << "Move event TODO update titles" << endl;
  }
  else
    evlog.log() << "Unhandled window event type: " << change << endl;
  
  delete json;
  return;
}

/**
 * Handles the incoming event. response is NOT freed.
 * returns 1 if event is relevant
 */
void handleEvent(I3State& i3State, uint32_t type, char* response)
{
  switch(type)
  {
  case I3_INVALID_TYPE:
    if(!die)
      evlog.log() << "Invalid packet received. Aborting" << endl;
    die = 1;
    break;
  case I3_IPC_EVENT_MODE:
    {
      pthread_mutex_lock(&i3State.mutex);
      JSON* json = JSON::parse(response);
      JSONObject& modeEvent = json->object();
      i3State.mode = modeEvent["change"].string();
      delete json;
      pthread_mutex_unlock(&i3State.mutex);
    }
    break;
  case I3_IPC_EVENT_WINDOW:
    handleWindowEvent(i3State, response);
    break;
  case I3_IPC_EVENT_WORKSPACE:
    handleWorkspaceEvent(i3State, response);
    break;
  case I3_IPC_EVENT_OUTPUT:
    evlog.log() << "Output event - Restarting application" << endl;
    exit_status = EXIT_RESTART;
    die = 1;
    break;
  default:
    evlog.log() << "Unhandled event type: " << type << endl;
    break;
  }
  
  if(i3State.valid == 0)
  {
    evlog.log() << "Invalid change after event " << type << " occured" << endl;
    if(response != NULL)
      evlog.log() << response << endl;
    die = 1;
  }
  
  if(response != NULL) 
    free(response);
}

struct event_listener_data
{
  I3State* i3State;
  int push_socket;
};

void* event_listener(void* data)
{
  try
  {

  evlog.log() << "Launching event listener" << endl;
  event_listener_data* eldp = (event_listener_data*)data;
  int push_socket = eldp->push_socket;
  I3State& i3State = *eldp->i3State;
  free(eldp);
  char abonnements[] = "[\"workspace\",\"mode\",\"output\",\"window\"]";
  sendMessage(push_socket, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, abonnements);
  uint32_t type;
  char* response = readMessage(push_socket, &type);
  handleEvent(i3State, type, response);
    
  evlog.log() << "Entering event listener loop" << endl;
  while(!die)
  {
    //this sleep prevents the application from dying because of SIGUSR1 spam.
    //on the other hand, the user can now crash, or at least DOS i3 with
    //events, which cannot be processed fast enough
    //could be replaced with a mutex...
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 10000000;
    nanosleep(&t, NULL);
  
    response = readMessage(push_socket, &type);
    handleEvent(i3State, type, response);
    
    while(!die && hasInput(push_socket, 1000))
    {
      response = readMessage(push_socket, &type);
      handleEvent(i3State, type, response);
    }
    
    pthread_cond_signal(&notifier);
  }
  evlog.log() << "Exiting event listener loop" << endl;
  
  shutdown(push_socket, SHUT_RDWR);
  
  evlog.log() << "Stopping event listener" << endl;
  
  }
  catch(JSONException& e)
  {
    cerr << e.what() << endl;
  }
  //pthread_exit(0); which is better?
  return nullptr; // this produces no warnings
}

void forkEventListener(I3State* i3, string& path)
{
  event_listener_data* data = (event_listener_data*)malloc(sizeof(event_listener_data));
  data->i3State = i3;
  data->push_socket = init_socket(path.c_str());
  pthread_create(&event_listener_thread, nullptr, &event_listener, (void*)data);
}

