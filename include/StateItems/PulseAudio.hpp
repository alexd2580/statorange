#ifndef STATEITEMS_VOLUME_HPP
#define STATEITEMS_VOLUME_HPP

#include <ostream>
#include <string>

#include <pulse/pulseaudio.h>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"

class PulseAudio final : public StateItem {
  private:
    pa_mainloop* pa_mainloop;
    pa_context* pa_context;

    bool is_connected;
    bool has_handled_sink_info;
    bool sink_info_has_changed;

    std::string port_name;
    bool is_mute;
    uint32_t volume;

    void log_pa_error();

    void connect();
    void handle_state_change();
    void handle_sink_info_response(pa_sink_info const& sink_info);
    void disconnect();

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar& bar, uint8_t display_index) override;

  public:
    explicit PulseAudio(JSON::Node const& item);
    virtual ~PulseAudio() override;
};

#endif
