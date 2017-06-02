#include "output.hpp"
#include <iostream>

#include "StateItem.hpp"
#include "StateItems/Battery.hpp"
#include "StateItems/CPU.hpp"
#include "StateItems/Date.hpp"
#include "StateItems/IMAPMail.hpp"
#include "StateItems/Net.hpp"
#include "StateItems/Space.hpp"
#include "StateItems/Volume.hpp"

using namespace std;

StateItem::StateItem(JSON const& item) : last_updated(0)
{
  module_name.assign(item["item"]);
  cooldown = (time_t)item["cooldown"];
  button = false;

  if(item.has("button"))
  {
    button_command.assign(item["button"]);
    button = true;
  }
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
    separate(Direction::left, Color::warn);
    cout << " Module " << module_name << " failed ";
    separate(Direction::left, Color::white_on_black);
  }
}

vector<StateItem*> StateItem::states;

/**
 * This method initializes the settings for each type of item.
 * If missing it will throw a JSONException here.
 * and reads the "order" component of the config, instantiating the items.
 */
void StateItem::init(JSON const& config)
{
  auto& order = config["order"];
  auto length = order.size();

  for(decltype(length) i = 0; i < length; i++)
  {
    auto& section = order[i];
    string item = section["item"];
    if(item.compare("CPU") == 0)
      states.push_back(new CPU(section));
    else if(item.compare("Battery") == 0)
      states.push_back(new Battery(section));
    else if(item.compare("Net") == 0)
      states.push_back(new Net(section));
    else if(item.compare("Date") == 0)
      states.push_back(new Date(section));
    else if(item.compare("Volume") == 0)
      states.push_back(new Volume(section));
    else if(item.compare("Space") == 0)
      states.push_back(new Space(section));
    else if(item.compare("IMAPMail") == 0)
      states.push_back(new IMAPMail(section));
  }
}

void StateItem::updates(void)
{
  for(auto i = states.begin(); i != states.end(); i++)
    (*i)->wrap_update();
}

void StateItem::forceUpdates(void)
{
  for(auto i = states.begin(); i != states.end(); i++)
    (*i)->force_update();
}

void StateItem::printState(void)
{
  for(auto i = states.begin(); i != states.end(); i++)
    (*i)->wrap_print();
}

void StateItem::deinit(void)
{
  for(auto i = states.begin(); i != states.end(); i++)
    delete(*i);
}
