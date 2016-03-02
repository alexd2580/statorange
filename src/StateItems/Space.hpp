#ifndef __PARTITIONSIZEXYZHEADER_LOL___
#define __PARTITIONSIZEXYZHEADER_LOL___

#include <string>
#include <vector>

#include "../JSON/jsonParser.hpp"
#include "../StateItem.hpp"
#include "../output.hpp"

struct SpaceItem
{
  std::string mount_point;
  Icon icon;
  float size;
  float used;
  std::string unit;
};

class Space : public StateItem, public Logger
{
private:
  std::vector<SpaceItem> items;
  bool getSpaceUsage(SpaceItem& dir);

  bool update(void);
  void print(void);

public:
  explicit Space(JSONObject& item);
  virtual ~Space() {}
};

#endif
