#include <cstring>
#include <ostream>
#include <utility> // pair

#include <pulse/pulseaudio.h>

#include "StateItems/PulseAudio.hpp"

#include "Lemonbar.hpp"

void PulseAudio::log_pa_error() {
    log() << "pa_context_connect() failed: " << pa_strerror(pa_context_errno(pa_context)) << std::endl;
}

void PulseAudio::connect() {
    pa_mainloop = pa_mainloop_new();
    if(pa_mainloop == nullptr) {
        log() << "pa_mainloop_new() failed." << std::endl;
        return;
    }

    pa_mainloop_api* pa_mainloop_api = pa_mainloop_get_api(pa_mainloop);
    if(pa_signal_init(pa_mainloop_api) != 0) {
        log() << "pa_signal_init() failed." << std::endl;
        return;
    }

    pa_context = pa_context_new(pa_mainloop_api, "PulseAudio Test");
    if(pa_context == nullptr) {
        std::cerr << "pa_context_new() failed." << std::endl;
        return;
    }

    auto set_state_callback = [](struct pa_context* context, void* userdata) {
        (void)context;
        static_cast<PulseAudio*>(userdata)->handle_state_change();
    };
    pa_context_set_state_callback(pa_context, set_state_callback, this);

    if(pa_context_connect(pa_context, nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr) < 0) {
        log_pa_error();
        return;
    }

    while(!is_connected) {
        pa_mainloop_iterate(pa_mainloop, 1, nullptr);
    }
}

void PulseAudio::handle_state_change() {
    switch(pa_context_get_state(pa_context)) {
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
        break;

    case PA_CONTEXT_READY:
        is_connected = true;
        break;

    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_TERMINATED:
        is_connected = false;
        break;
    case PA_CONTEXT_FAILED:
        is_connected = false;
        log_pa_error();
        break;
    }
}

std::pair<bool, bool> PulseAudio::handle_sink_info_response(pa_sink_info const* sink_info_ptr) {
    if(sink_info_ptr == nullptr) {
        return {false, true};
    }

    auto const sink_info = *sink_info_ptr;
    auto const active_port_ptr = sink_info.active_port;
    if(active_port_ptr == nullptr) {
        return {false, true};
    }

    bool changed = false;

    pa_sink_port_info const& active_port = *active_port_ptr;
    std::string new_port_name(active_port.description);
    changed |= new_port_name != port_name;
    port_name = new_port_name;

    auto const volume_average = pa_cvolume_avg(&(sink_info.volume));
    uint32_t const new_volume = 100 * volume_average / PA_VOLUME_NORM;
    changed |= new_volume != volume;
    volume = new_volume;

    bool const new_is_mute = sink_info.mute != 0;
    changed |= new_is_mute != is_mute;
    is_mute = new_is_mute;

    return {true, changed};
}

void PulseAudio::disconnect() {
    if(pa_context != nullptr) {
        pa_context_unref(pa_context);
    }
    if(pa_mainloop != nullptr) {
        pa_mainloop_free(pa_mainloop);
    }
}

std::pair<bool, bool> PulseAudio::update_raw() {
    auto get_sink_info_callback = [](struct pa_context* context, const pa_sink_info* sink_info,
                                                           int eol, void* userdata) {
        (void)context;
        (void)eol;
        auto data = *static_cast<std::pair<PulseAudio*, std::pair<bool, bool>*>*>(userdata);
        *(data.second) = data.first->handle_sink_info_response(sink_info);
    };

    // Pass data to the callback.
    auto result = std::make_pair(false, false);
    auto userdata = std::make_pair(this, &result);
    pa_operation* a = pa_context_get_sink_info_list(pa_context, get_sink_info_callback, &userdata);

    if(a == nullptr) {
        return {false, true};
    }

    while(pa_operation_get_state(a) != PA_OPERATION_DONE) {
        pa_mainloop_iterate(pa_mainloop, 1, nullptr);
    }

    return result;
}

void PulseAudio::print_raw(Lemonbar& bar, uint8_t display_num) {
    (void)display_num;
    auto temp_colors = Lemonbar::section_colors<uint32_t>(volume, 101, 150);
    bar.separator(Lemonbar::Separator::left, temp_colors.first, temp_colors.second);
    std::string icon(is_mute ? "\ufc5d" : volume < 33 ? "\ufa7e" : volume < 66 ? "\ufa7f" : "\ufa7d");
    bar() << icon << " " << port_name << " " << volume << "% ";
    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black);
}

PulseAudio::PulseAudio(JSON::Node const& item) : StateItem(item) {
    pa_mainloop = nullptr;
    pa_context = nullptr;

    is_connected = false;

    connect();

    port_name = "No Port";
    is_mute = false;
    volume = 0;
}

PulseAudio::~PulseAudio() { disconnect(); }
