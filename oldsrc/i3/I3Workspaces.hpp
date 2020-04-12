#ifndef __I3WKSPCXYZHEADER_LOL___
#define __I3WKSPCXYZHEADER_LOL___

#include <memory>
#include <ostream>
#include <string>

#include "json_parser.hpp"
#include "StateItem.hpp"
#include "i3/I3State.hpp"

class I3Workspaces : public StateItem
{
  private:
    I3State i3;

    /**
     * Given an i3 container tries to get its name.
     */
    static std::string get_window_name(JSON::Node const& container);

    /**
     * The following are handler functions for each type of event. These extract
     * the information from the JSON::Node object and update the state of `i3`.
     */
    bool invalid_event(void);
    bool mode_event(char const* response);
    bool window_event(char const* response);
    bool workspace_event(char const* response);
    bool output_event(char const* response);

};

#endif
