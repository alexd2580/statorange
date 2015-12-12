#ifndef __VOLUMEXYZHEADER__
#define __VOLUMEXYZHEADER__

#include<string>
#include"../StateItem.hpp"

class Volume : public StateItem
{
private:
  static std::string get_volume;

  //VOLUME
  bool mute;
  int volume;

  bool update(void);
  void print(void);

public:
  static void settings(JSONObject&);
  Volume(JSONObject& item);
  virtual ~Volume() {};
};

#endif