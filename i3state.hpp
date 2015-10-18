#ifndef __I3STATE_HEADER___
#define __I3STATE_HEADER___

#include<cstdint>
#include<string>
#include<pthread.h>

using namespace std;

#define MAX_DISP_LEN 20
#define MAX_NAME_LEN 50

typedef struct Output_ Output;
struct Output_
{
  string name;
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
  string name;
  char visible;
  char focused;
  char urgent;
  Output* output;
  
  long focusedAppID; // -1 indicates unknown
  string focusedApp; // undefined when unknown
};

class I3State
{
private:
  int fd;
  
public:
  pthread_mutex_t mutex; //TODO private

  I3State(std::string path);
  ~I3State();
  char valid;
  
  string mode;

  uint8_t outputCount;
  Output* outputs;
  
  uint8_t wsCount;
  Workspace* workspaces;
  
  Workspace* focusedWorkspace;
  
  void updateOutputs(void);
  void workspaceInit(uint8_t num);
  void updateWorkspaceStatus(void);
  void workspaceEmpty(uint8_t num);
};


#endif
