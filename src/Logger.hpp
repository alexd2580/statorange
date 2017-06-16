#ifndef __LOGGINGALLTHESTUFFS__
#define __LOGGINGALLTHESTUFFS__

#include <memory>
#include <ostream>
#include <string>

class Logger
{
private:
  std::string const logname;
  std::ostream& log_ostream;

public:
  Logger(std::string lname);
  virtual ~Logger() = default;

  static std::ostream& log(std::string const&, std::ostream& ostream);
  static std::ostream& log(std::string const&);
  std::ostream& log(void) const;
  void log_errno(void);

};

#ifdef _MSC_VER
#define __METHOD__ __FUNCTION__
#elif __GNUC__
#define __METHOD__ __PRETTY_FUNCTION__
#else
#define __METHOD__ __func__
#error i have read a lot of bad things in code and this is one of them.
#endif

#define ANON_LOG (Logger::log(__METHOD__))

class LoggerManager
{
  friend class Logger;

private:
  static std::unique_ptr<LoggerManager> instance;

  std::ostream& log_stream;
  LoggerManager(std::ostream& ostr);

public:
  static void set_stream(std::ostream& ostr);
};

#endif
