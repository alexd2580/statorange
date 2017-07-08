#ifndef __I3WKSPCXYZHEADER_LOL___
#define __I3WKSPCXYZHEADER_LOL___

#include <memory>
#include <ostream>
#include <string>

#include "../json_parser.hpp"
#include "../StateItem.hpp"
#include "../i3/I3State.hpp"

class I3Workspaces : public StateItem
{
  private:
    I3State i3;
    int event_socket;

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

    /**
    * Handles the incoming i3 packet. Assumes ownership of the response string.
    */
    bool handle_message(uint32_t type, std::unique_ptr<char[]> response);

    /**
     * Sends a subscribe message to i3, requesting notification on useful
     * events.
     */
    void subscribe_to_events(void);

    /**
     * Requests the tree from i3 and verifies that the current state of i3,
     * which is generated from the events, matches the tree, updating things
     * where necessary. TODO Verify that the state is always valid by updating
     * and disable this.
     */
    bool update(void);
    void print(std::ostream&, uint8_t);

  protected:
    /**
     * Performs a nonblocking read and processes the messages on `fd`.
     */
    bool handle_events(int fd);

  public:
    I3Workspaces(JSON::Node const&);
    virtual ~I3Workspaces(void) = default;
};

#endif
