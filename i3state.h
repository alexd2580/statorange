#ifndef __I3STATE_HEADER___
#define __I3STATE_HEADER___

#include<pthread.h>
#include<stdint.h>

#define MAX_DISP_LEN 20
#define MAX_NAME_LEN 50

typedef struct Output_ Output;
struct Output_
{
  char name[MAX_DISP_LEN];
  int posX;
  int posY;
};

typedef struct Workspace_ Workspace;
struct Workspace_
{
  /**
   * unique number of workspace
   * nobody ever used more than 256 workspaces!
   */
  uint8_t num; 
  char name[MAX_NAME_LEN];
  char visible;
  char focused;
  char urgent;
  Output* output;
  
  long focusedAppID; // -1 indicates unknown
  char focusedApp[MAX_NAME_LEN]; // undefined when unknown
};

typedef struct I3State_ I3State;
struct I3State_
{
  pthread_mutex_t mutex;
  int fd;
  char valid;
  
  char mode[MAX_NAME_LEN];

  uint8_t outputCount;
  Output* outputs;
  
  uint8_t wsCount;
  Workspace* workspaces;
  
  Workspace* focusedWorkspace;
};

void init_i3State(I3State* i3, char* path);
void update_i3StateOutputs(I3State* i3);
void update_i3StateWorkspaceInit(I3State* i3, uint8_t num);
void update_i3StateWorkspaceStatus(I3State* i3);
void update_i3StateWorkspaceEmpty(I3State* i3, uint8_t num);
void free_i3State(I3State* i3);

#endif
