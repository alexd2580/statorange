/**
 * ONLY FOR PRIVATE USE!
 * NEITHER VALIDATED, NOR EXCESSIVELY TESTED
 */

#define _POSIX_C_SOURCE 200809L

#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<time.h>
#include<pthread.h>
#include<signal.h>
#include<sys/socket.h>

#include"jsonParser.h"
#include"jsonSearch.h"
#include"output.h"
#include"systemState.h"
#include"i3state.h"
#include"i3-ipc.h"
#include"i3-ipc-constants.h"
#include"strUtil.h"

#define getSocket "i3 --get-socketpath"

/******************************************************************************/
/********************************** GLOBALS ***********************************/

I3State i3State;
SystemState systemState;

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
void terminate_handler(int signum)
{
  (void)signum;
  die = 1;
  
  pthread_t this_ = pthread_self();
  if(this_ == main_thread)
  {
    fprintf(stderr, "main() received signal %d\n", signum);
    fprintf(stderr, "Interrupting event_listener()\n");
    pthread_kill(event_listener_thread, SIGINT);
  }
  else
  {
    fprintf(stderr, "event_listener() received signal %d\n", signum);
    fprintf(stderr, "Notifying main()\n");
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

void handleWorkspaceEvent(char* response)
{
  char* ptr = getJSONObjectField(response, "change", 6);
  size_t evTypeLen;
  char* eventType;
  ptr = getJSONString(ptr, &eventType, &evTypeLen);
  
  if(evTypeLen == 4 && strncmp(eventType, "init", 4) == 0)
  {
    GET_DISPLAY_NUM
    update_i3StateWorkspaceInit(&i3State, (uint8_t)n);
  }
  else if(evTypeLen == 5 && strncmp(eventType, "focus", 5) == 0)
    update_i3StateWorkspaceStatus(&i3State);
  else if(evTypeLen == 6 && strncmp(eventType, "urgent", 6) == 0)
    update_i3StateWorkspaceStatus(&i3State);
  else if(evTypeLen == 5 && strncmp(eventType, "empty", 5) == 0)
  {
    GET_DISPLAY_NUM
    update_i3StateWorkspaceEmpty(&i3State, (uint8_t)n);
  }
  else
  {
    fprintf(stderr, "Unhandled workspace event type: %.*s", (int)evTypeLen, eventType);
    fprintf(stderr, "\n");
  }
  return;
}

void handleWindowEvent(char* response)
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
        storeString(i3State.workspaces[i].focusedApp, MAX_NAME_LEN, strPtr, strLen);
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
    storeString(fw->focusedApp, MAX_NAME_LEN, strPtr, strLen);
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
    fprintf(stderr, "Fullscreen mode TODO if anything at all...\n");
  }
  else if(cLen == 4 && strncmp(cStr, "move", 4) == 0)
  {
    fprintf(stderr, "Move event TODO update titles\n");
  }
  else
  {
    fprintf(stderr, "Unhandled window event type: %.*s", (int)cLen, cStr);
    fprintf(stderr, "\n");
  }
  
  freeJSON((JSONSomething*)windowEvent);
  return;
}

/**
 * Handles the incoming event. response is NOT freed.
 * returns 1 if event is relevant
 */
void handleEvent(uint32_t type, char* response)
{
  switch(type)
  {
  case I3_INVALID_TYPE:
    if(!die)
      fprintf(stderr, "Invalid packet received. Aborting\n");
    die = 1;
    break;
  case I3_IPC_EVENT_MODE:
    {
      pthread_mutex_lock(&i3State.mutex);
      JSONObject* modeEvent = (JSONObject*)parseJSON(response);
      JSONString* modeString = (JSONString*)getField(modeEvent, "change");
      size_t modeLen = 0;
      char* mode = getString(modeString, &modeLen);
      i3State.mode[0] = '\0';  
      if(strncmp("default", mode, 7) != 0 || modeLen > 7)
      {
        memcpy(i3State.mode, mode, modeLen);
        i3State.mode[modeLen] = '\0';
      }
      freeJSON((JSONSomething*)modeEvent);
      pthread_mutex_unlock(&i3State.mutex);
    }
    break;
  case I3_IPC_EVENT_WINDOW:
    handleWindowEvent(response);
    break;
  case I3_IPC_EVENT_WORKSPACE:
    handleWorkspaceEvent(response);
    break;
  case I3_IPC_EVENT_OUTPUT:
    fprintf(stderr, "Output event - Restarting application\n");
    exit_status = EXIT_RESTART;
    die = 1;
    break;
  default:
    fprintf(stderr, "Unhandled event type: %u\n", type);
    break;
  }
  
  if(i3State.valid == 0)
  {
    fprintf(stderr, "Invalid change after event %ud occured\n", type);
    if(response != NULL)
    {
      fprintf(stderr, "%s", response);
      fprintf(stderr, "\n");
    }
    die = 1;
  }
  
  if(response != NULL) 
    free(response);
}

void* event_listener(void* push_socket_ptr)
{
  int push_socket = *((int*)push_socket_ptr);
  char abonnements[] = "[\"workspace\",\"mode\",\"output\",\"window\"]";
  sendMessage(push_socket, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, abonnements);
  uint32_t type;
  char* response = readMessage(push_socket, &type);
  handleEvent(type, response);
    
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
    handleEvent(type, response);
    
    while(!die && hasInput(push_socket, 1000))
    {
      fprintf(stderr, "Handling subsequent event\n");
      response = readMessage(push_socket, &type);
      handleEvent(type, response);
    }
    
    pthread_cond_signal(&notifier);
  }
  
  //pthread_exit(0); which is better?
  return NULL; // this produces no warnings
}

/******************************************************************************/

int main(void)
{
  char path[100] = { 0 };
  execute(getSocket, NULL, NULL, path, 100);
  char* p=path;
  while(*p != '\n') p++;
  *p = '\0';

  init_colors();
  init_systemState(&systemState);
  init_i3State(&i3State, path);
  update_i3StateOutputs(&i3State);    
  
  pthread_cond_init(&notifier, NULL);
  pthread_mutex_init(&mutex, NULL);

  int push_socket = init_socket(path);
  pthread_create(&event_listener_thread, NULL, &event_listener, (void*)&push_socket);
  main_thread = pthread_self();
  
  struct sigaction terminate_handler_action;
  terminate_handler_action.sa_handler = terminate_handler;               
  sigemptyset(&terminate_handler_action.sa_mask);
  //sa.sa_flags = SA_RESTART | SA_NODEFER;
  terminate_handler_action.sa_flags = 0;
  sigaction(SIGINT, &terminate_handler_action, NULL);
  sigaction(SIGTERM, &terminate_handler_action, NULL);
  //signal(SIGINT, sigint_handler);

  struct sigaction sigusr1_handler_action;
  sigusr1_handler_action.sa_handler = notify_handler;               
  sigemptyset(&sigusr1_handler_action.sa_mask);
  //sa.sa_flags = SA_RESTART | SA_NODEFER;
  sigusr1_handler_action.sa_flags = 0;
  sigaction(SIGUSR1, &sigusr1_handler_action, NULL);
  //signal(SIGUSR1, notify_handler);

  pthread_mutex_lock(&mutex);
  while(!die)
  {
    update_systemState(&systemState);
    if(force_update)
    {
      update_constSystemState(&systemState);
      force_update = 0;
    }
    
    pthread_mutex_lock(&i3State.mutex);
    if(!i3State.valid)
      die = 1;
    else
    {
      echoPrimaryLemon(&i3State, &systemState, i3State.outputs, 0);
      for(uint8_t i=1; i<i3State.outputCount; i++)
        echoSecondaryLemon(&i3State, &systemState, i3State.outputs+i, i);
      putchar('\n');
      fflush(stdout);
    }
    pthread_mutex_unlock(&i3State.mutex);

    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += 5;
    pthread_cond_timedwait(&notifier, &mutex, &abstime);
  }
  pthread_mutex_unlock(&mutex);
  
  pthread_join(event_listener_thread, NULL);
  shutdown(push_socket, SHUT_RDWR);
  
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&notifier);
  
  free_systemState(&systemState);
  free_i3State(&i3State);
  
  return exit_status; 
}

