
#include "Date.hpp"
#include <chrono> // std::chrono::system_clock
#include <ctime>
#include <iomanip> // std::put_time
#include <ostream>
#include <sstream>

#include "../output.hpp"
#include "../util.hpp"

using namespace std;

Date::Date(JSON const& item) : StateItem(item), format(item["format"])
{
}

using std::chrono::system_clock;

bool Date::update(void)
{
    time_t tt = system_clock::to_time_t(system_clock::now());
    struct tm* ptm = localtime(&tt);
    ostringstream o;
    print_time(o, ptm, format.c_str());
    time = o.str();
    return true;
}

void Date::print(ostream& out, uint8_t)
{
    BarWriter::separator(
        out, BarWriter::Separator::left, BarWriter::Coloring::active);
    out << icon << ' ' << time << ' ';
    BarWriter::separator(
        out, BarWriter::Separator::left, BarWriter::Coloring::white_on_black);
}
