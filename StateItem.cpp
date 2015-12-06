
#include<iostream>
#include"output.hpp"

#include"StateItem.hpp"
#include"StateItems/Battery.hpp"
#include"StateItems/CPU.hpp"
#include"StateItems/Date.hpp"
#include"StateItems/Net.hpp"
#include"StateItems/Space.hpp"
#include"StateItems/Volume.hpp"

using namespace std;

StateItem::StateItem(string name, time_t cd) : 
  module_name(name), cooldown(cd)
{
  last_updated = 0;
  valid = false;
}

void StateItem::wrap_update(void)
{
  time_t now = time(nullptr);
  if(now > last_updated + cooldown)
  {
    last_updated = now;
    valid = update();
  }
}

void StateItem::force_update(void)
{
  time_t now = time(nullptr);
  last_updated = now;
  valid = update();
}

void StateItem::wrap_print(void)
{
  if(valid)
    print();
  else
  {
    separate(Left, warn_colors);
    cout << " Module " << module_name << " failed ";
    separate(Left, white_on_black);
  }
}

vector<StateItem*> StateItem::states;

void StateItem::init(void)
{
  //INIT CODE HERE
  states.push_back(new Volume());
  states.push_back(new Net("eth0", Ethernet));
  states.push_back(new Net("wlan0", Wireless));
  states.push_back(new Battery());
  states.push_back(new CPU());
  vector<string> mpoints;
  mpoints.push_back("/");
  mpoints.push_back("/home");
  states.push_back(new Space(mpoints));
  states.push_back(new Date());
}

void StateItem::updates(void)
{
  for(auto i=states.begin(); i!=states.end(); i++)
    (*i)->wrap_update();
}

void StateItem::forceUpdates(void)
{
  for(auto i=states.begin(); i!=states.end(); i++)
    (*i)->force_update();
}

void StateItem::printState(void)
{
  for(auto i=states.begin(); i!=states.end(); i++)
    (*i)->wrap_print();
}

void StateItem::close(void)
{
  for(auto i=states.begin(); i!=states.end(); i++)
    delete (*i);
}


