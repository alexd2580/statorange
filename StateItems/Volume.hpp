#ifndef __VOLUMEXYZHEADER__
#define __VOLUMEXYZHEADER__

#include<string>
#include"../StateItem.hpp"

class Volume : public StateItem
{
private:
  //VOLUME
  bool mute;
  int volume;
  
  std::string const amixer_cmd;
  std::string const alsamixer_cmd;
  
  void performUpdate(void);
  void print(void);
public:
  Volume();
  virtual ~Volume() {};
}; 

#endif

