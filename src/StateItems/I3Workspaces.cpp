
#include <cstdlib>
#include <cstring>
#include <ostream>

#include "../output.hpp"
#include "../util.hpp"
#include "I3Workspaces.hpp"

using namespace std;
using BarWriter::Separator;
using BarWriter::Coloring;

I3Workspaces::I3Workspaces(JSON const& item) : StateItem(item)
{
}

bool I3Workspaces::update(void)
{
}

void I3Workspaces::print(ostream& out, uint8_t)
{
}
