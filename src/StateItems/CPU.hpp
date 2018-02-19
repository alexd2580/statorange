#ifndef __CPUXYZHEADER_LOL___
#define __CPUXYZHEADER_LOL___

#include <ostream>
#include <string>
#include <vector>

#include "../Lemonbar.hpp"
#include "../StateItem.hpp"
#include "../json_parser.hpp"

class CPU final : public StateItem {
  private:
    std::string const load_file_path;
    std::vector<std::string> temp_file_paths;

    std::vector<uint32_t> cpu_temps;
    float cpu_load;

    // Returns the success of the operation.
    static bool read_line(std::string const& path, char* data, uint32_t num);

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar&, uint8_t) override;

  public:
    explicit CPU(JSON::Node const& item);
    virtual ~CPU() override = default;
};

#endif
