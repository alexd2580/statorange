/**
 * ONLY FOR PRIVATE USE!
 * NEITHER VALIDATED, NOR EXCESSIVELY TESTED
 */

#define _POSIX_C_SOURCE 200809L

#include<cstdlib>
#include<cstring>
#include<iostream>
#include<ctime>
#include<csignal>

#include<unistd.h>
#include<pthread.h>
#include<sys/socket.h>

#include"jsonParser.hpp"
#include"jsonSearch.hpp"
#include"output.hpp"
#include"systemState.hpp"
#include"i3state.hpp"
#include"i3-ipc.hpp"
#include"i3-ipc-constants.hpp"
#include"strUtil.hpp"

#define getSocket "i3 --get-socketpath"

using namespace std;

/******************************************************************************/
/********************************** GLOBALS ***********************************/

pthread_t main_thread;
pthread_t event_listener_thread;
pthread_cond_t notifier;
pthread_mutex_t mutex;

//#define EXIT_SUCCESS 0
#define EXIT_RESTART 4

volatile sig_atomic_t die = 0;
volatile sig_atomic_t exit_status = EXIT_SUCCESS;
volatile sig_atomic_t force_update = 1;

/******************************************************************************/
void term_handler(int signum)
{
  (void)signum;
  die = 1;
  
  pthread_t this_ = pthread_self();
  if(this_ == main_thread)
  {
    cerr << "main() received signal " << signum << endl;
    cerr << "Interrupting event_listener()" << endl;
    pthread_kill(event_listener_thread, SIGINT);
  }
  else
  {
    cerr << "event_listener() received signal " << signum << endl;
    cerr << "Notifying main()" << endl;
    pthread_cond_signal(&notifier);
  }
  return;
}

/**
 * Used to force a systemstate update including
 * "constant" states
 */
void notify_handler(int signum)
{
  signal(signum, notify_handler);
  force_update = 1;
  pthread_cond_signal(&notifier);
  
  pthread_t this_ = pthread_self();
  if(this_ == event_listener_thread)
    pthread_kill(main_thread, SIGINT);
  return;
}

#define GET_DISPLAY_NUM \
  ptr = getJSONObjectField(response, "current", 7); \
  ptr = getJSONObjectField(ptr, "num", 3); \
  double n; \
  getJSONNumber(ptr, &n);

void handleWorkspaceEvent(I3State& i3State, char* response)
{
  char* ptr = getJSONObjectField(response, "change", 6);
  size_t evTypeLen;
  char* eventType;
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
    cerr << "Unhandled workspace event type: ";
    cerr.write(eventType, (int)evTypeLen);
    cerr << endl;
  }
  return;
}

void handleWindowEvent(I3State& i3State, char* response)
{
  JSONObject* windowEvent = (JSONObject*)parseJSON(response);
  
  JSONString* changeString = (JSONString*)getField(windowEvent, "change");
  size_t cLen;
  char* cStr = getString(changeString, &cLen);
  
  JSONObject* container = (JSONObject*)getField(windowEvent, "container");
  JSONNumber* appId = (JSONNumber*)getField(container, "id");
  double appIdD = getNumber(appId);
  long appIdL = (long)appIdD;
  
  /** New events are also accompanied by focus events if necessary */
  //cLen == 3 && strncmp(cStr, "new", 3) == 0
  
  /**
   * On a title update check if the application is focused 
   * on any visible workspace. If yes -> update (id does not change)
   */
  if(cLen == 5 && strncmp(cStr, "title", 5) == 0)
  {
    for(uint8_t i=0; i<i3State.wsCount; i++)
    {
      if(i3State.workspaces[i].focusedAppID == appIdL)
      {
        JSONString* name = (JSONString*)getField(container, "name");
        size_t strLen;
        char* strPtr = getString(name, &strLen);
        i3State.workspaces[i].focusedApp.assign(strPtr, strLen); // TODO length
        break;
      }
    }
  }
  /** Copy the title/id of the currently focused window to it's WS. */  
  else if(cLen == 5 && strncmp(cStr, "focus", 5) == 0)
  {
    Workspace* fw = i3State.focusedWorkspace;
    JSONString* name = (JSONString*)getField(container, "name");
    size_t strLen;
    char* strPtr = getString(name, &strLen);
    fw->focusedApp.assign(strPtr, strLen); //TODO length
    fw->focusedAppID = appIdL;
  }
  /** 
   * usually after a close event a focus event is issued, 
   * if there is a focused window. therefore delete the focus here.
   * I don't know on which workspace the application closed.
   */
  else if(cLen == 5 && strncmp(cStr, "close", 5) == 0)
  {
    for(uint8_t i=0; i<i3State.wsCount; i++)
    {
      if(i3State.workspaces[i].focusedAppID == appIdL)
      {
        i3State.workspaces[i].focusedAppID = -1;
        i3State.workspaces[i].focusedApp[0] = '\0';
        break;
      }
    }
  }
  else if(cLen == 15 && strncmp(cStr, "fullscreen_mode", 15) == 0)
  {
    cerr << "Fullscreen mode TODO if anything at all..." << endl;
  }
  else if(cLen == 4 && strncmp(cStr, "move", 4) == 0)
  {
    cerr << "Move event TODO update titles" << endl;
  }
  else
  {
    cerr << "Unhandled window event type: ";
    cerr.write(cStr, (int)cLen);
    cerr << endl;
  }
  
  freeJSON((JSONSomething*)windowEvent);
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
      cerr << "Invalid packet received. Aborting" << endl;
    die = 1;
    break;
  case I3_IPC_EVENT_MODE:
    {
      pthread_mutex_lock(&i3State.mutex);
      JSONObject* modeEvent = (JSONObject*)parseJSON(response);
      JSONString* modeString = (JSONString*)getField(modeEvent, "change");
      size_t modeLen = 0;
      char* mode = getString(modeString, &modeLen);
      i3State.mode.assign(mode, modeLen);
      freeJSON((JSONSomething*)modeEvent);
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
    cerr << "Output event - Restarting application" << endl;
    exit_status = EXIT_RESTART;
    die = 1;
    break;
  default:
    cerr << "Unhandled event type: " << type << endl;
    break;
  }
  
  if(i3State.valid == 0)
  {
    cerr << "Invalid change after event " << type << " occured" << endl;
    if(response != NULL)
    {
      cerr << response << endl;
    }
    die = 1;
  }
  
  if(response != NULL) 
    free(response);
}

struct event_listener_data
{
  I3State& i3StateRef;
  int push_socket;
};

void* event_listener(void* data)
{
  struct event_listener_data* eldp = (struct event_listener_data*)data;
  int push_socket = eldp->push_socket;
  I3State& i3State =eldp->i3StateRef;
  char abonnements[] = "[\"workspace\",\"mode\",\"output\",\"window\"]";
  sendMessage(push_socket, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, abonnements);
  uint32_t type;
  char* response = readMessage(push_socket, &type);
  handleEvent(i3State, type, response);
    
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
      cerr << "Handling subsequent event" << endl;
      response = readMessage(push_socket, &type);
      handleEvent(i3State, type, response);
    }
    
    pthread_cond_signal(&notifier);
  }
  
  //pthread_exit(0); which is better?
  return nullptr; // this produces no warnings
}

/******************************************************************************/

int main(void)
{
  string path = execute(getSocket, nullptr, nullptr);
  for(size_t i=0; i<path.length(); i++)
    if(path[i] == '\n')
      path[i] = '\0';

  init_colors();

  I3State i3State(path);
  i3State.updateOutputs();
  SystemState sysState;
  
  pthread_cond_init(&notifier, nullptr);
  pthread_mutex_init(&mutex, nullptr);

  int push_socket = init_socket(path.c_str());
  event_listener_data data
  {
    .i3StateRef = i3State,
    .push_socket = push_socket
  };
  pthread_create(&event_listener_thread, nullptr, &event_listener, (void*)&data);
  main_thread = pthread_self();
  
  struct sigaction term_handler_action;
  term_handler_action.sa_handler = term_handler;               
  sigemptyset(&term_handler_action.sa_mask);
  //sa.sa_flags = SA_RESTART | SA_NODEFER;
  term_handler_action.sa_flags = 0;
  sigaction(SIGINT, &term_handler_action, nullptr);
  sigaction(SIGTERM, &term_handler_action, nullptr);
  //signal(SIGINT, sigint_handler);

  struct sigaction sigusr1_handler_action;
  sigusr1_handler_action.sa_handler = notify_handler;               
  sigemptyset(&sigusr1_handler_action.sa_mask);
  //sa.sa_flags = SA_RESTART | SA_NODEFER;
  sigusr1_handler_action.sa_flags = 0;
  sigaction(SIGUSR1, &sigusr1_handler_action, nullptr);
  //signal(SIGUSR1, notify_handler);

  pthread_mutex_lock(&mutex);
  while(!die)
  {
    sysState.update();
    if(force_update)
    {
      sysState.updateAll();
      force_update = 0;
    }
    
    pthread_mutex_lock(&i3State.mutex);
    if(!i3State.valid)
      die = 1;
    else
    {
      echoPrimaryLemon(i3State, sysState, i3State.outputs, 0);
      for(uint8_t i=1; i<i3State.outputCount; i++)
        echoSecondaryLemon(i3State, sysState, i3State.outputs+i, i);
      cout << endl;
      cout.flush();
    }
    pthread_mutex_unlock(&i3State.mutex);

    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += 5;
    pthread_cond_timedwait(&notifier, &mutex, &abstime);
  }
  pthread_mutex_unlock(&mutex);
  
  pthread_join(event_listener_thread, nullptr);
  shutdown(push_socket, SHUT_RDWR);
  
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&notifier);
  
  return exit_status; 
}

