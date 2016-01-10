#include <iostream>
#include <cstring>

#include "Volume.hpp"
#include "../output.hpp"
#include "../util.hpp"

using namespace std;

/******************************************************************************/
/******************************************************************************/

string Volume::deprecated_get_volume = "";

void Volume::settings(JSONObject& section)
{
  deprecated_get_volume = section["deprecated_get_volume"].string();
}

Volume::Volume(JSONObject& item) : StateItem(item)
{
  card = item["card"].string();
  mixer = item["mixer"].string();

  mute = true;
  volume = 0;
}

#define __VOLUME_WITH_ALSA__
#ifdef __VOLUME_WITH_ALSA__

#include <alsa/asoundlib.h>
bool Volume::update(void)
{
  long min, max;
  snd_mixer_t* handle;
  snd_mixer_selem_id_t* sid;
  // const char *card = "default";
  // const char *selem_name = "Master";

  snd_mixer_open(&handle, 0);
  snd_mixer_attach(handle, card.c_str());
  snd_mixer_selem_register(handle, NULL, NULL);
  snd_mixer_load(handle);

  snd_mixer_selem_id_alloca(&sid);
  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, mixer.c_str()); // selem_name
  snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

  if(elem == nullptr)
  {
    snd_mixer_close(handle);
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
  return true;
}

#else

bool Volume::update(void)
{
  string output = execute(get_volume);
  char const* c = output.c_str();

  while(*c != '\0')
  {
    int unused;
    char on[5] = {0};
    int matched =
        sscanf(c, "  Front Left: Playback %d [%d%%] [%s", &unused, &volume, on);
    // TODO other formats
    if(matched == 3)
    {
      mute = strncmp(on, "on]", 3) != 0;
      return true;
    }

    while(*c != '\n' && *c != '\0')
      c++;
    if(*c == '\n')
      c++;
  }

  return false;
}

#endif

void Volume::print(void)
{
  separate(Left, neutral_colors);
  print_icon(icon_vol);
  if(mute)
    cout << " Mute ";
  else
    cout << ' ' << volume << "% ";
  separate(Left, white_on_black);
}
