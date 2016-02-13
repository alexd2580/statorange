#ifndef __PARTITIONSIZEXYZHEADER_LOL___
#define __PARTITIONSIZEXYZHEADER_LOL___

#include <string>
#include <vector>
#include "../StateItem.hpp"
#include "../JSON/jsonParser.hpp"

struct SpaceItem
{
  std::string mount_point;
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
  static void settings(JSONObject&) {}
  explicit Space(JSONObject& item);
  virtual ~Space() {}
};

#endif
