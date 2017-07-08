#ifndef __CPUXYZHEADER_LOL___
#define __CPUXYZHEADER_LOL___

#include <ostream>
#include <string>
#include <vector>

#include "../json_parser.hpp"
#include "../StateItem.hpp"
#include "../util.hpp"

class CPU : public StateItem
{
  private:
    std::string const load_file_path;
    std::vector<std::string> temp_file_paths;

    std::vector<int> cpu_temps;
    float cpu_load;

    bool update(void) override;
    void print(std::ostream&, uint8_t) override;

  public:
    CPU(JSON::Node const& item);
    virtual ~CPU(void) = default;
};

#endif
