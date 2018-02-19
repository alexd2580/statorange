#include <functional>
#include <iostream>

#include "Logger.hpp"

std::reference_wrapper<std::ostream> Logger::default_ostream(std::cerr);
