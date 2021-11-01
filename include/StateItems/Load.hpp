#ifndef STATEITEMS_LOAD_HPP
#define STATEITEMS_LOAD_HPP

#include <ostream>
#include <string>
#include <vector>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"

class Load final : public StateItem {
  private:
    std::string const load_file_path;
    std::vector<std::string> temp_file_paths;

    std::vector<uint32_t> cpu_temps;
    int64_t uptime;
    float cpu_load;
    uint64_t total_ram;
    uint64_t free_ram;

    // Returns the success of the operation.
    static bool read_line(std::string const& path, char* data, uint32_t num);

    void read_memory_stats();

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar& bar, uint8_t display) override;

  public:
    explicit Load(JSON::Node const& item);
};

#endif
