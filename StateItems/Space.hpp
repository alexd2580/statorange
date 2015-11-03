#ifndef __PARTITIONSIZEXYZHEADER_LOL___
#define __PARTITIONSIZEXYZHEADER_LOL___

#include<string>
#include<vector>
#include"../StateItem.hpp"

struct SpaceItem
{
  std::string mountPoint;
  std::string size;
  std::string used;
};

class Space : public StateItem
{
private:
  std::vector<SpaceItem> items;
  void getSpaceUsage(SpaceItem& dir);
  
  static std::string const getSpace;

  void performUpdate(void);
  void print(void);

public:
  Space(std::vector<std::string>&);
  virtual ~Space();
};

#endif
