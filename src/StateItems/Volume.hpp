#ifndef __VOLUMEXYZHEADER__
#define __VOLUMEXYZHEADER__

#include <string>

#include "../StateItem.hpp"
#include "../output.hpp"

class Volume : public StateItem
{
private:
  Icon icon;
  std::string card;
  std::string mixer;

  // VOLUME
  bool mute;
  unsigned short volume;

  bool update(void);
  void print(void);

public:
  Volume(JSONObject& item);
  virtual ~Volume() {}
};

#endif
