#ifndef __CPUXYZHEADER_LOL___
#define __CPUXYZHEADER_LOL___

#include<string>
#include"../StateItem.hpp"

class CPU : public StateItem
{
private:
  bool cached;
  std::string printString;
  
  //CPU
  int cpu_temp;
  float cpu_load;
  
  static std::string const temp_file_loc;
  static std::string const load_file_loc;
  std::string const sysmgr_cmd;

  void performUpdate(void);
  void print(void);
public:
  CPU();
  virtual ~CPU() {};
};

#endif
