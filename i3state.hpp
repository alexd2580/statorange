#ifndef __I3STATE_HEADER___
#define __I3STATE_HEADER___

#include<cstdint>
#include<string>
#include<vector>

#include<pthread.h>

#define MAX_DISP_LEN 20
#define MAX_NAME_LEN 50

struct Output
{
  std::string name;
  int posX;
  int posY;
};

struct Workspace
{
  /**
   * unique number of workspace
   * nobody ever used more than 256 workspaces!
   */
  uint8_t num; 
  std::string name;
  char visible;
  char focused;
  char urgent;
  uint8_t output;
  
  long focusedAppID; // -1 indicates unknown
  std::string focusedApp; // undefined when unknown
};

class I3State
{
private:
  int fd;
  
public:
  pthread_mutex_t mutex; //TODO private

  I3State(std::string path);
  ~I3State();
  bool valid;
  
  std::string mode;

  std::vector<Output> outputs;
  std::vector<Workspace> workspaces;
  
  size_t focusedWorkspace;
  
  void parseOutputs(void);
  void parseWorkspaces(void);
  
  void updateOutputs(void);
  void workspaceInit(uint8_t num);
  void updateWorkspaceStatus(void);
  void workspaceEmpty(uint8_t num);
};


#endif
