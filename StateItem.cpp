
#include"StateItem.hpp"
#include"StateItems/Battery.hpp"
#include"StateItems/CPU.hpp"
#include"StateItems/Date.hpp"
#include"StateItems/Net.hpp"
#include"StateItems/Space.hpp"

using namespace std;

StateItem::StateItem(time_t cd) : 
  cooldown(cd)
{
  last_updated = 0;
}

StateItem::~StateItem()
{
}

void StateItem::update(void)
{
  time_t now = time(nullptr);
  if(now > last_updated + cooldown)
  {
    last_updated = now;
    performUpdate();
  }
}

void StateItem::forceUpdate(void)
{
  time_t now = time(nullptr);
  last_updated = now;
  performUpdate();
}

vector<StateItem*> StateItem::states;

void StateItem::init(void)
{
  //INIT CODE HERE
  states.push_back(new CPU());
  states.push_back(new Net("eth0", Ethernet));
  states.push_back(new Net("wlan0", Wireless));
  states.push_back(new Battery());
  vector<string> mpoints;
  mpoints.push_back("/");
  mpoints.push_back("/home");
  states.push_back(new Space(mpoints));
  states.push_back(new Date());
}

void StateItem::updates(void)
{
  for(auto i=states.begin(); i!=states.end(); i++)
    (*i)->update();
}

void StateItem::forceUpdates(void)
{
  for(auto i=states.begin(); i!=states.end(); i++)
    (*i)->forceUpdate();
}

void StateItem::printState(void)
{
  for(auto i=states.begin(); i!=states.end(); i++)
    (*i)->print();
}

void StateItem::close(void)
{
  for(auto i=states.begin(); i!=states.end(); i++)
    delete (*i);
}


