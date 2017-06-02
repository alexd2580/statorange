#ifndef __CPUXYZHEADER_LOL___
#define __CPUXYZHEADER_LOL___

#include <string>
#include <vector>

#include "../StateItem.hpp"
#include "../JSON/json_parser.hpp"
#include "../util.hpp"

class CPU : public StateItem, public Logger
{
private:
  std::vector<std::string> temp_file_paths;
  std::string load_file_path;

  bool cached;
  std::string print_string;

  // CPU
  std::vector<int> cpu_temps;
  float cpu_load;

  bool update(void);
  void print(void);

public:
  CPU(JSON const& item);
  virtual ~CPU(void) = default;
};

#endif
