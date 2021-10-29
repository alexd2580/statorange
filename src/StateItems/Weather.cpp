#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <sstream>

#include <fmt/chrono.h>

#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

#include "StateItems/Weather.hpp"


Weather::Weather(JSON::Node const& item) : StateItem(item) {
    // auto this_cooldown = item["cooldown"].number<time_t>();
    // min_cooldown = std::chrono::seconds(std::min(min_cooldown.count(), this_cooldown));
}

std::map<std::string, std::string> const Weather::icons{
    // nf-weather-thunderstorm
    // 389 Moderate or heavy rain in area with thunder
    // 386 Patchy light rain in area with thunder
    // 200 Thundery outbreaks in nearby
    {"389", "\ue31d"},
    {"386", "\ue31d"},
    {"200", "\ue31d"},

    // nf-weather-showers
    // 266 Light drizzle
    // 263 Patchy light drizzle
    {"266", "\ue319"},
    {"263", "\ue319"},

    // 293 Patchy light rain
    // 176 Patchy rain nearby
    // 296 Light rain
    // 353 Light rain shower
    {"293", "\ue319"},
    {"176", "\ue319"},
    {"296", "\ue319"},
    {"353", "\ue319"},

    // nf-weather-rain
    // 302 Moderate rain
    // 299 Moderate rain at times
    // 356 Moderate or heavy rain shower
    {"302", "\ue318"},
    {"299", "\ue318"},
    {"356", "\ue318"},

    // 308 Heavy rain
    // 305 Heavy rain at times
    // 359 Torrential rain shower
    {"308", "\ue318"},
    {"305", "\ue318"},
    {"359", "\ue318"},

    // nf-weather-snow
    // 179 Patchy snow nearby
    // 323 Patchy light snow
    // 326 Light snow
    // 368 Light snow showers
    {"179", "\ue31a"},
    {"323", "\ue31a"},
    {"326", "\ue31a"},
    {"368", "\ue31a"},

    // nf-weather-snow_wind
    // 395 Moderate or heavy snow in area with thunder
    // 392 Patchy light snow in area with thunder
    {"395", "\ue35e"},
    {"392", "\ue35e"},

    // 329 Patchy moderate snow
    // 332 Moderate snow
    // 338 Heavy snow
    // 371 Moderate or heavy snow showers
    // 335 Patchy heavy snow
    {"329", "\ue35e"},
    {"332", "\ue35e"},
    {"338", "\ue35e"},
    {"371", "\ue35e"},
    {"335", "\ue35e"},

    // 227 Blowing snow
    // 230 Blizzard
    {"227", "\ue35e"},
    {"230", "\ue35e"},

    // nf-weather-sleet
    // 365 Moderate or heavy sleet showers
    // 362 Light sleet showers
    // 350 Ice pellets
    // 320 Moderate or heavy sleet
    // 317 Light sleet
    // 185 Patchy freezing drizzle nearby
    // 182 Patchy sleet nearby
    // 377 Moderate or heavy showers of ice pellets
    // 311 Light freezing rain
    // 374 Light showers of ice pellets
    // 284 Heavy freezing drizzle  w
    // 281 Freezing drizzle
    // 314 Moderate or Heavy freezing rain
    {"365", "\ue3ad"},
    {"362", "\ue3ad"},
    {"350", "\ue3ad"},
    {"320", "\ue3ad"},
    {"317", "\ue3ad"},
    {"185", "\ue3ad"},
    {"182", "\ue3ad"},
    {"377", "\ue3ad"},
    {"311", "\ue3ad"},
    {"374", "\ue3ad"},
    {"284", "\ue3ad"},
    {"281", "\ue3ad"},
    {"314", "\ue3ad"},

    // nf-weather-fog
    // 260 Freezing fog
    // 248 Fog
    // 143 Mist
    {"260", "\ue313"},
    {"248", "\ue313"},
    {"143", "\ue313"},

    // nf-weather-cloud
    // 122 Overcast
    // 119 Cloudy
    // 116 Partly Cloudy
    {"122", "\ue33d"},
    {"119", "\ue33d"},
    {"116", "\ue33d"},

    // 113 Clear/Sunny
    // nf-weather-night_clear
    {"113-day", "\ue30d"},
    // nf-weather-day_sunny
    {"113-night", "\ue32b"},
};

decltype(auto) am_pm_to_duration_since_midnight(std::string const& input) {
    auto const ctoi = [](char c) { return c - '0'; };
    auto hours = 10 * ctoi(input[0]) + ctoi(input[1]);
    auto minutes = 10 * ctoi(input[3]) + ctoi(input[4]);
    if (input[6] == 'P') {
        hours += 12;
    }

    return std::chrono::minutes{hours * 60 + minutes};
}

std::pair<bool, bool> Weather::update_raw() {
    // Do request
    try {
        log() << "Updating weather..." << std::endl;
        std::stringstream ss;

        curlpp::Cleanup cleanup;
        curlpp::Easy request;
        request.setOpt<curlpp::options::Url>("https://wttr.in?format=j1");
        request.setOpt(curlpp::options::WriteStream(&ss));
        request.perform();

        auto const response = ss.str();
        auto const response_json = JSON::Node(response.c_str());
        auto const current_condition = response_json["current_condition"][0];

        temperature = current_condition["temp_C"].string();
        condition_code = current_condition["weatherCode"].string();
        condition = current_condition["weatherDesc"][0]["value"].string();

        auto const astronomy = response_json["weather"][0]["astronomy"][0];
        auto const sunrise_str = astronomy["sunrise"].string();
        auto const sunset_str = astronomy["sunset"].string();

        midnight_to_sunrise = am_pm_to_duration_since_midnight(sunrise_str);
        midnight_to_sunset = am_pm_to_duration_since_midnight(sunset_str);

        log() << fmt::format("Done => {}, {}°C", condition, temperature) << std::endl;
    }

    // return {success, changed};
    catch(curlpp::RuntimeError& e) {
        log() << "Error: " << e.what() << std::endl;
        return {false, true};
    }

    catch(curlpp::LogicError& e) {
        log() << "Error: " << e.what() << std::endl;
        return {false, true};
    }

    catch(std::string& message) {
        log() << message << std::endl;
        return {false, true};
    }

    return {true, true};
}

void Weather::print_raw(Lemonbar& bar, uint8_t display_arg) {
    (void)display_arg;

    int temp = std::stoi(temperature);
    auto const colors = Lemonbar::section_colors(abs(temp - 15), 10, 15);
    bar.separator(Lemonbar::Separator::left, colors.first, colors.second);

    // Clang does not support current_zone =/
    time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm* now = localtime(&tt);
    std::chrono::minutes since_midnight(60 * now->tm_hour + now->tm_min);

    bool is_day = midnight_to_sunrise <= since_midnight && since_midnight < midnight_to_sunset;
    auto const& code = condition_code != "113" ? condition_code : condition_code + (is_day ? "-day" : "-night");

    bar() << fmt::format("{} {} {}°C ", icons.at(code), condition, temperature);
    bar.separator(Lemonbar::Separator::left);

    if (since_midnight < midnight_to_sunrise) {
        auto const now_to_sunrise = midnight_to_sunrise - since_midnight;
        bar() << fmt::format("\ue34c in {:%H:%M} ", now_to_sunrise);
    } else if (since_midnight < midnight_to_sunset) {
        auto const now_to_sunrise = midnight_to_sunset - since_midnight;
        bar() << fmt::format("\ue34d in {:%H:%M} ", now_to_sunrise);
    } else {
        auto const now_to_sunrise = midnight_to_sunrise + std::chrono::days(1) - since_midnight;
        bar() << fmt::format("\ue34c in {:%H:%M} ", now_to_sunrise);
    }

    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black);
}
