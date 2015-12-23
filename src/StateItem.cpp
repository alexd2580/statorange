
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

StateItem::StateItem(JSONObject& item)
{
  module_name.assign(item["item"].string());
  cooldown = (time_t)item["cooldown"].number();
  JSON* cmd = item.has("button");
  if((button = (cmd != nullptr)))
    button_command = cmd->string();

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
  {
    if(button)
    {
      startButton(button_command, cout);
      print();
      stopButton(cout);
    }
    else
      print();
  }
  else
  {
    separate(Left, warn_colors);
    cout << " Module " << module_name << " failed ";
    separate(Left, white_on_black);
  }
}

vector<StateItem*> StateItem::states;

/**
 * This method initializes the settings for each type of item.
 * If missing it will throw a JSONException here.
 * and reads the "order" component of the config, instantiating the items.
 */
void StateItem::init(JSONObject& config)
{
  //Date::settings(config["Date"].object());
  //Net::settings(config["Net"].object());
  CPU::settings(config["CPU"].object());
  Battery::settings(config["Battery"].object());
  Volume::settings(config["Volume"].object());
  Space::settings(config["Space"].object());

  JSONArray& order = config["order"].array();
  size_t length = order.length();

  for(size_t i=0; i<length; i++)
  {
    JSONObject& section = order[i].object();
    string item = section["item"].string();
    if(item.compare("CPU") == 0)
        states.push_back(new CPU(section));
    if(item.compare("Battery") == 0)
        states.push_back(new Battery(section));
    if(item.compare("Net") == 0)
        states.push_back(new Net(section));
    if(item.compare("Date") == 0)
        states.push_back(new Date(section));
    if(item.compare("Volume") == 0)
        states.push_back(new Volume(section));
    if(item.compare("Space") == 0)
        states.push_back(new Space(section));
  }
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
