/**
 * ONLY FOR PRIVATE USE!
 * NEITHER VALIDATED, NOR EXCESSIVELY TESTED
 */

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>

#include <cassert>
#include <csignal>
#include <cstdlib>
#include <cstring>

#include <pwd.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "json_parser.hpp"
#include "utils/io.hpp"

#include "StateItems/Battery.hpp"
#include "StateItems/Date.hpp"
#include "StateItems/I3.hpp"
#include "StateItems/Load.hpp"
#include "StateItems/Net.hpp"
#include "StateItems/Space.hpp"
#include "StateItems/PulseAudio.hpp"
#include "StateItems/Weather.hpp"

#include "Lemonbar.hpp"
#include "Logger.hpp"
#include "StateItem.hpp"

/******************************************************************************/

// typedef struct sigaction Sigaction;
// Sigaction mk_handler(sighandler_t handler)
// {
//     Sigaction handler_action;
//     handler_action.sa_handler = handler;
//     sigemptyset(&handler_action.sa_mask);
//     // sa.sa_flags = SA_RESTART | SA_NODEFER;
//     handler_action.sa_flags = 0;
//     return handler_action;
// }
//
// void register_handler(int sig, Sigaction& handler)
// {
//     sigaction(sig, &handler, nullptr);
// }

/******************************************************************************/

class Statorange : Logger {
  private:
    JSON::Node config_json;

    int signal_fd;

    bool force_update = true;
    bool dead = false;
    bool restart = false;

    bool show_failed_modules;

    using StateItems = std::vector<std::unique_ptr<StateItem>>;
    StateItems left_items;
    StateItems center_items;
    StateItems right_items;

    Lemonbar bar;

    void init_item(JSON::Node const& json_item, StateItems& section) {
#define CREATE_ITEM(itemclass)                                                                                         \
    if(item == #itemclass) {                                                                                           \
        log() << "Creating item of type " << item << std::endl;                                                        \
                                                                                                                       \
        section.push_back(std::unique_ptr<StateItem>(new itemclass(json_item)));                                       \
    }

        try {
            std::string const& item = json_item["item"].string();
            CREATE_ITEM(Date)
            CREATE_ITEM(Space)
            CREATE_ITEM(Load)
            CREATE_ITEM(Battery)
            CREATE_ITEM(Net)
            CREATE_ITEM(I3)
            CREATE_ITEM(PulseAudio)
            CREATE_ITEM(Weather)
        } catch(std::string const& error) {
            // Errors will be ignored.
            log() << error << std::endl;
        }
    }

    void init_section(std::string const& section_name, StateItems& section) {
        if(config_json[section_name].exists()) {
            for(auto const& json_item : config_json[section_name].array()) {
                init_item(json_item, section);
            }
        }
    }

    bool apply_config() {
        show_failed_modules = config_json["show failed modules"].boolean();

        init_section("left", left_items);
        init_section("center", center_items);
        init_section("right", right_items);

        return true;
    }

    void setup_signal_handler() {
        log() << "Setting up signal handlers" << std::endl;

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGQUIT);
        sigaddset(&mask, SIGUSR1);

        /* Block signals so that they aren't handled
           according to their default dispositions */

        if(sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
            log() << "Failed to set up signal blocker, signals may terminate the "
                     "application"
                  << std::endl;
        }

        signal_fd = signalfd(-1, &mask, 0);
        if(signal_fd == -1) {
            log() << "Failed to set up signal handler, signals will be ignored" << std::endl;
            signal_fd = 0;
        }
    }

    // Chech the signal FD and handle the signals.
    void handle_signals() {
        int32_t fd_has_input = 0;
        while(true) {
            fd_has_input = has_input(signal_fd);
            if(fd_has_input == 0) {
                // No more data here.
                return;
            }
            if(fd_has_input < 0) {
                log_errno();
                log() << "failed to select readable fd" << std::endl;
                return;
            }

            log() << "Signal buffer contains signal info" << std::endl;
            struct signalfd_siginfo signal_info {};
            constexpr size_t signal_info_size = sizeof(struct signalfd_siginfo);
            ssize_t s = read_all(signal_fd, static_cast<char*>(static_cast<void*>(&signal_info)), signal_info_size);
            if(s != signal_info_size) {
                log_errno();
                log() << "Failed to read struct signalfd_siginfo from signal fd" << std::endl;
                log() << "Aborting signal handling due to broken read" << std::endl;
                break;
            }

            switch(signal_info.ssi_signo) {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                log() << "Got exit signal " << signal_info.ssi_signo << std::endl;
                dead = true;
                break;
            case SIGUSR1:
                log() << "Got SIGUSR1" << std::endl;
                force_update = true;
                break;
            default:
                log() << "Got unexpected signal " << signal_info.ssi_signo << std::endl;
                break;
            }
        }
    }

    bool update() {
        bool updated = false;
        for(auto& state : left_items) {
            updated = state->update(force_update) || updated;
        }
        for(auto& state : center_items) {
            updated = state->update(force_update) || updated;
        }
        for(auto& state : right_items) {
            updated = state->update(force_update) || updated;
        }
        force_update = false;
        return updated;
    }

    void print_section(Lemonbar::Alignment a, StateItems const& section, uint8_t display_number) {
        bar.align_begin(a);
        for(auto& state : section) {
            state->print(bar, display_number);
        }
        bar.align_end();
    }

    void print() {
        const uint8_t num_output_displays = 2;
        for(uint8_t i = 0; i < num_output_displays; i++) {
            bar.display_begin(i);
            print_section(Lemonbar::Alignment::left, left_items, i);
            print_section(Lemonbar::Alignment::center, center_items, i);
            print_section(Lemonbar::Alignment::right, right_items, i);
            bar.display_end();
        }
        bar() << std::endl;
    }

  public:
    explicit Statorange(std::ostream& ostr, JSON::Node const& config) : Logger("Main"), config_json(config), bar(ostr) {
        signal_fd = 0;
        show_failed_modules = true;
    }

    // Returns `true` if it should be restarted.
    bool run(int argc, char* argv[]) {
        (void)argc;
        (void)argv;
        log() << "Launching Statorange" << std::endl;

        if(!apply_config()) {
            return false;
        }

        setup_signal_handler();

        while(!dead && !restart) {
            if(update()) {
                print();
            }

            StateItem::wait_for_events(signal_fd);
            handle_signals();
        }

        return restart;
    }
};

int main(int argc, char* argv[]) {
    Logger main("main");

    main.log() << "Searching config" << std::endl;

    JSON::Node config_json;
    struct passwd* pw = getpwuid(getuid());
    std::string home_dir(pw->pw_dir);
    std::string config_name("config.json");
    std::vector<std::string> config_paths{"./" + config_name, home_dir + "/.config/statorange/" + config_name,
                                          home_dir + "/.statorange/" + config_name};

    for(auto const& path : config_paths) {
        main.log() << "Testing path: " << path << std::endl;
        std::string config_string;
        if(load_file(path, config_string)) {
            try {
                config_json = JSON::Node(config_string.c_str());
            } catch(char const* err_msg) {
                main.log() << "Failed to load config from '" << path << "':" << std::endl << err_msg << std::endl;
            }
        }
    }

    if(!config_json.exists()) {
        main.log() << "Failed to load config" << std::endl;
        return 1;
    }


    if(argc == 2 && std::string(argv[1]) == "--dry") {
        Statorange app(std::cout, config_json);
        return app.run(argc, argv) ? 1 : 0;
    }

    // The following comes from the `nerd-fonts-complete` AUR package, then taken from fc-list.

    // I don't exactly understand why, and there's probably a good explanation for this, but here's the gist:
    // lemonbar (xft/pango <- my lemonbar port) does not work with the new patched versions of nerd-fonts (2.1.0)
    // the rendering breaks down and double-width characters get cut off.
    // Therefore i need to install the old version of nerd fonts (2.0.0-1 through 2.0.0-5).
    // These fonts get correctly displayed in `font-manager` and look fine in the bar, but beware.
    // For some reason the old version of FiraCode is also broken. Therefore in case i actually wanted to use FiraCode
    // with the bar i would have to use `FuraCode` (Yes, FUUUUUUU-RA code), which seems to be the old version
    // (2.0.0-5) of what is now called FiraCode, although in the old version it was also caled `FiraCode`...
    // I fucking hate font config.

    const std::string text_font = config_json["font"].string();
        // (R"(-f "UbuntuMono Nerd Font:size=12")");
    // const std::string text_font(R"(-f "FuraMono Nerd Font:size=15")"); // More or less ok
    // const std::string text_font(R"(-f "FuraMono Nerd Font Mono:size=15")"); //Because mono - condensed icons
    // const std::string text_font(R"(-f "FuraCode Nerd Font:size=10")"); // looks ok
    // const std::string text_font(R"(-f "FuraCode Nerd Font Mono:size=15")"); // Because mono - condensed icons
    // const std::string text_font(R"(-f "FiraMono Nerd Font:size=15")"); // Completely broken.
    // const std::string text_font(R"(-f "FiraMono Nerd Font Mono:size=15")"); // Because mono - icons condensed.
    // const std::string text_font(R"(-f "FiraCode Nerd Font:size=15")"); // Correct character size, but icons cut off.
    // const std::string text_font(R"(-f "FiraCode Nerd Font Mono:size=15")"); // Because mono - icons condensed
    const std::string lemonbar_cmd("lemonbar -f \"" + text_font + "\" -a 30 -u -2");
    auto lemonbar_pipe = run_command(lemonbar_cmd, "w");
    FileStream<UniqueFile> lemonbar_streambuf(std::move(lemonbar_pipe));
    std::ostream lemonbar_stream(&lemonbar_streambuf);

    Statorange app(lemonbar_stream, config_json);
    return app.run(argc, argv) ? 1 : 0;
}
