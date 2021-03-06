#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>

#include "Logger.hpp"
#include "util.hpp"

using namespace std;

std::unique_ptr<LoggerManager> LoggerManager::instance;

LoggerManager::LoggerManager(ostream& ostr) : log_stream(ostr) {}

void LoggerManager::set_stream(ostream& ostr)
{
  instance = std::unique_ptr<LoggerManager>(new LoggerManager(ostr));
}

Logger::Logger(string lname)
    : logname(lname), ostream(LoggerManager::instance->log_stream)
{
}

ostream& Logger::log(string const& logname, std::ostream& ostream)
{
  time_t tt = chrono::system_clock::to_time_t(chrono::system_clock::now());
  struct tm ptm;
  localtime_r(&tt, &ptm);
  char const* const log_time_format = "%Y-%m-%d %H:%M ";
  return print_time(ostream, &ptm, log_time_format) << logname << ' ';
}

ostream& Logger::log(string const& logname)
{
  return log(logname, LoggerManager::instance->log_stream);
}

ostream& Logger::log() const { return log(logname, ostream); }

void Logger::log_errno(void)
{
  log() << "errno = " << errno << endl;
  log() << "Error description is : " << strerror(errno) << endl;
}
