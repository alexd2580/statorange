#include <alsa/asoundlib.h>
#include <cstring>
#include <iostream>

#include "Volume.hpp"
#include "../util.hpp"
#include "../JSON/JSONException.hpp"

using namespace std;

/******************************************************************************/
/******************************************************************************/

Volume::Volume(JSON const& item) : StateItem(item)
{
  try
  {
    icon = parse_icon(item["icon"]);
  }
  catch(JSONException&)
  {
    icon = Icon::no_icon;
  }

  card.assign(item["card"]);
  mixer.assign(item["mixer"]);

  mute = true;
  volume = 0;
}

bool Volume::update(void)
{
  long min, max;
  snd_mixer_t* handle;
  snd_mixer_selem_id_t* sid =
      (snd_mixer_selem_id_t*)calloc(1, snd_mixer_selem_id_sizeof());
  // const char *card = "default";
  // const char *selem_name = "Master";

  snd_mixer_open(&handle, 0);
  snd_mixer_attach(handle, card.c_str());
  snd_mixer_selem_register(handle, nullptr, nullptr);
  snd_mixer_load(handle);

  //  snd_mixer_selem_id_alloca(&sid);

  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, mixer.c_str()); // selem_name
  snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

  if(elem == nullptr)
  {
    snd_mixer_close(handle);
    free(sid);
    return false;
  }

  if(snd_mixer_selem_has_playback_switch(elem))
  {
    int mute_val;
    snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &mute_val);
    mute = mute_val == 0;
  }
  else
    mute = false;

  long vol_val;
  snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &vol_val);
  snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

  snd_mixer_close(handle);

  volume = (unsigned short)(100 * vol_val / max);
  free(sid);
  return true;
}

void Volume::print(void)
{
  separate(Direction::left, Color::neutral);
  cout << icon;
  if(mute)
    cout << " Mute ";
  else
    cout << ' ' << volume << "% ";
  separate(Direction::left, Color::white_on_black);
}
