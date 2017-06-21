#ifndef __CPUXYZHEADER_LOL___
#define __CPUXYZHEADER_LOL___

#include <ostream>
#include <string>
#include <vector>

#include "../JSON/json_parser.hpp"
#include "../StateItem.hpp"
#include "../util.hpp"

class CPU : public StateItem
{
  private:
    std::vector<std::string> temp_file_paths;
    std::string load_file_path;

    // CPU
    std::vector<int> cpu_temps;
    float cpu_load;

    bool update(void);
    void print(std::ostream&, uint8_t);

  public:
    CPU(JSON const& item);
    virtual ~CPU(void) = default;
};

#endif
