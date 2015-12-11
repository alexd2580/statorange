#ifndef __CPUXYZHEADER_LOL___
#define __CPUXYZHEADER_LOL___

#include<string>
#include"../StateItem.hpp"
#include"../JSON/jsonParser.hpp"

class CPU : public StateItem
{
private:
  static std::string temp_file_loc;
  static std::string load_file_loc;

  bool cached;
  std::string print_string;

  //CPU
  int cpu_temp;
  float cpu_load;

  bool update(void);
  void print(void);

public:
  static void settings(JSONObject&);
  CPU(JSONObject& item);
  virtual ~CPU() {};
};

#endif
