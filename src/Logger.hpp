#ifndef __LOGGINGALLTHESTUFFS__
#define __LOGGINGALLTHESTUFFS__

#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>

#include "util.hpp"

#ifdef _MSC_VER
#define __METHOD__ __FUNCTION__
#elif __GNUC__
#define __METHOD__ __PRETTY_FUNCTION__
#else
#define __METHOD__ __func__
#error i have read a lot of bad things in code and this is one of them.
#endif

class Logger
{
  private:
    static std::reference_wrapper<std::ostream> default_ostream;

    std::string const log_name;
    std::reference_wrapper<std::ostream> log_ostream;

  public:
    static void set_default_ostream(std::ostream& ostr)
    {
        default_ostream = std::ref(ostr);
    }

    void set_ostream(std::ostream& ostr) { log_ostream = std::ref(ostr); }

    Logger(std::string lname, std::ostream& ostr)
        : log_name(lname), log_ostream(ostr)
    {
    }

    Logger(std::string lname) : Logger(lname, default_ostream) {}

    virtual ~Logger() = default;

    static std::ostream& log(std::string const& lname, std::ostream& ostr)
    {
        time_t tt = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        struct tm ptm;
        localtime_r(&tt, &ptm);
        char const* const log_time_format = "%Y-%m-%d %H:%M ";
        return print_time(ostr, ptm, log_time_format)
               << "\t[" << lname << "]\t";
    }

    static std::ostream& log(std::string const& lname)
    {
        return log(lname, default_ostream);
    }

    std::ostream& log() const { return log(log_name, log_ostream); }

    std::ostream& operator()() const { return log(); }

    void log_errno()
    {
        (*this)() << "errno = " << errno << std::endl;
        (*this)() << "Error description is : " << strerror(errno) << std::endl;
    }
};

#define ANON_LOG (Logger::log(__METHOD__))

#endif
