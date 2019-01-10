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
// #include "StateItems/IMAPMail.hpp"
// #include "StateItems/Volume.hpp"
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

    bool load_config() {
        log() << "Searching config" << std::endl;

        struct passwd* pw = getpwuid(getuid());
        std::string home_dir(pw->pw_dir);
        std::string config_name("config.json");
        std::vector<std::string> config_paths{"./" + config_name, home_dir + "/.config/statorange/" + config_name,
                                              home_dir + ".statorange/" + config_name};

        for(auto const& path : config_paths) {
            log() << "Testing path: " << path << std::endl;
            std::string config_string;
            if(load_file(path, config_string)) {
                try {
                    config_json = JSON::Node(config_string.c_str());
                } catch(char const* err_msg) {
                    log() << "Failed to load config from '" << path << "':" << std::endl << err_msg << std::endl;
                }
            }
        }

        if(!config_json.exists()) {
            log() << "Failed to load config" << std::endl;
            return false;
        }

        return true;
    }

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
            // else if(item == "Volume")
            //     return new Volume(json_item);
            // else if(item == "IMAPMail")
            //     return new IMAPMail(json_item);
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
        const uint8_t num_output_displays = 3;
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
    explicit Statorange(std::ostream& ostr) : Logger("Main"), config_json(""), bar(ostr) {
        signal_fd = 0;
        show_failed_modules = true;
    }

    // Returns `true` if it should be restarted.
    bool run(int argc, char* argv[]) {
        (void)argc;
        (void)argv;
        log() << "Launching Statorange" << std::endl;

        if(!load_config()) {
            return false;
        }
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
    if(argc == 2 && std::string(argv[1]) == "--dry") {
        Statorange app(std::cout);
        return app.run(argc, argv);
    } else {
        // const std::string normal_font("-f -misc-fixed-medium-r-semicondensed--12------iso10646-1");
        const std::string text_font("-f -xos4-terminus2-medium-r-normal--12------iso8859-1");
        const std::string icon_font("-f -xos4-terminusicons2mono-medium-r-normal--12------iso8859-1");
        const std::string lemonbar_cmd("lemonbar " + text_font + " " + icon_font + " -a 30 -u -1");
        const std::string font_path_minus("xset fp- /usr/local/lib/statorange/misc");
        const std::string font_path_plus("xset fp+ /usr/local/lib/statorange/misc");
        const std::string lemonbar_with_fonts(font_path_minus + ";" + font_path_plus + ";" + lemonbar_cmd);
        auto lemonbar_pipe = run_command(lemonbar_with_fonts, "w");
        FileStream<UniqueFile> lemonbar_streambuf(std::move(lemonbar_pipe));
        std::ostream lemonbar_stream(&lemonbar_streambuf);

        Statorange app(lemonbar_stream);
        return app.run(argc, argv);
    }
}
