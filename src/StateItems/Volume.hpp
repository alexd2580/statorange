#ifndef __VOLUMEXYZHEADER__
#define __VOLUMEXYZHEADER__

#include <string>
#include "../StateItem.hpp"

class Volume : public StateItem
{
private:
  static std::string deprecated_get_volume;
  std::string card;
  std::string mixer;

  // VOLUME
  bool mute;
  unsigned short volume;

  bool update(void);
  void print(void);

public:
  static void settings(JSONObject&);
  Volume(JSONObject& item);
  virtual ~Volume(){}
};

#endif
