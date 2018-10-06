#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>

#include "utils/io.hpp"

#ifdef _MSC_VER
#define __METHOD__ __FUNCTION__
#elif __GNUC__
#define __METHOD__ __PRETTY_FUNCTION__
#else
#define __METHOD__ __func__
#error i have read a lot of bad things in code and this is one of them.
#endif

class Logger {
  private:
    static std::reference_wrapper<std::ostream> default_ostream;

    std::string const name;
    std::reference_wrapper<std::ostream> ostream;

  public:
    static void set_default_ostream(std::ostream& ostr) { default_ostream = std::ref(ostr); }

    Logger(std::string new_name, std::ostream& new_ostr) : name(std::move(new_name)), ostream(new_ostr) {}
    explicit Logger(std::string const& new_name) : Logger(new_name, default_ostream) {}
    Logger(Logger const& logger) = default;
    Logger(Logger&& logger) = default;
    virtual ~Logger() = default;

    Logger& operator=(Logger const& logger) = delete;
    Logger& operator=(Logger&& logger) = delete;

    void set_ostream(std::ostream& ostr) { ostream = std::ref(ostr); }

    static std::ostream& log(std::string const& name, std::ostream& ostr) {
        time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm ptm {};
        localtime_r(&tt, &ptm);
        char const* const time_format = "%Y-%m-%d %H:%M ";
        return print_time(ostr, ptm, time_format) << "\t[" << name << "]\t";
    }
    static std::ostream& log(std::string const& name) { return log(name, default_ostream); }
    std::ostream& log() const { return log(name, ostream); }
    std::ostream& operator()() const { return log(); } // NOLINT

    void log_errno() {
        operator()() << "errno = " << errno << std::endl;
        operator()() << "Error description is : " << strerror(errno) << std::endl;
    }
};

#define ANON_LOG (Logger::log(__METHOD__))

#endif
