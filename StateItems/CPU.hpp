#ifndef __CPUXYZHEADER_LOL___
#define __CPUXYZHEADER_LOL___

#include<string>
#include"../StateItem.hpp"

class CPU : public StateItem
{
private:
  //CPU
  int cpu_temp;
  float cpu_load;
  
  static std::string temp_file_loc;
  static std::string load_file_loc;

  void performUpdate(void);
  void print(void);
public:
  CPU();
  virtual ~CPU();
};

#endif
