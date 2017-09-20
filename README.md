# **Statorange**

![](screenshot.png)

This is a status generator for i3 designed to replace i3bar/i3status.

## Prerequisites

Obviously, this software does NOT run on windows.
Statorange is (not anymore) tightly bound to the really awesome window manager i3,
Also, bar (lemonbar) is required for the actual panel.

The predefined status items use the following programs/files:

* Battery<br>
  * The [missing] file `/sys/class/power_supply/BAT0/uevent` and the ability to read files.

* Volume<br>
  * `alsamixer`, which opens when you click the volume section.

* CPU<br>
  * temperature: `/sys/bus/acpi/devices/LNXTHERM:00/thermal_zone/temp`<br>
      or `/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp2_input`
  * load: `/proc/loadavg`
  * `htop` pops up when this section is clicked.

* GMail<br>
  * `chromium`, for viewing g-mails upon clicking.

Also, since lemonbar does not provide a trayer section something like trayer
is advisable. You can add a shortcut to `toggle_trayer.sh` (i chose mod+t)
to toggle trayer, for when you really need it (you won't use it a lot,
except for nm-applet or wicd to connect to a network GRAPHICALLY).

### Required Packaged Software
* `i3-wm`
* `g++`/`clang++`
* `cmake`
* `git`
* `make`
* `alsa-utils`/`libasound2-dev`
* `libssl-dev`

### (Optional)
* `chromium` or any other browser
* `trayer`
* `htop`
* `terminator` or another terminal emulator (for certain features)

### Found on GitHub
* <a href=https://github.com/LemonBoy/bar>`bar` (`lemonbar`)</a> : https://github.com/LemonBoy/bar.git

## Compiling and installing

It's a cmake project, so -
```shell
mkdir build
cd build
cmake ..
make
sudo make install
```

## Launching

Probably you want to replace i3bar/i3status with statorange/lemonbar.
Change the bar section in your i3 config (~/.i3/config or ~/.config/i3/config):
```shell
bar {
    # Kills the launcher script, then the executable which may still be running and then launcher the bar.
    i3bar_command killall statorange_launcher; killall statorange; statorange_launcher
}
```

## Config

The config (`config.json`) is written in JSON. An example configuration can be found in `config.example.json`.

* static settings for the state items (CPU, Battery, Volume, Space, Date, Net, GMail).
* `order` - as the name implies, the order is important.
The objects in this list specify the sections on the right side of the bar.
Each object has the following fields:
  * `item` - the name of the item (CPU, Battery, ...)
  * `icon` - the name of the icon ("cpu", "wlan", ... see `src/output.hpp` for details)
        The icon will not automatically be displayed.
  * `cooldown` - the delay between state updates.
  * [optional] `button` the command to execute when the section is pressed.

Also some objects require additional fields:
* Net
  * `type` - ethernet or wireless
  * `interface` - the interface. Available interfaces can be displayed by running `/sbin/ifconfig`.
  * `show` - what address to show (ipv4, ipv6, none, both, ipv6_fallback)
* Space
  * `mount_points` - an array of mount points. I have /home as a separate partition.
Available mount points can be queried with `df -h`.
* Volume
  * `card` - the soundcard (put default there)
  * `mixer` - probably you want to put "Master" there
* Date
  * `format` - <a href=http://www.cplusplus.com/reference/iomanip/put_time/>the format in which to print the date.</a>
* GMail
  * `username` - Your GMail account name (with or without the @...)
  * `password` - Unfortunately, your password in plaintext. Don't worry, it's only transmitted to certified servers via SSL.
  * `mailbox` - The name of the mailbox to watch. "INBOX" is THE inbox.
  * Also you probably shouldn't set the update interval of GMail to less than 10 minutes, or you might get blocked.

After configuring statorange it needs to be restarted (when running via the launcher -> `killall statorange`).

## Inner structure

Statorange sleeps most of the time. Every `min_delay` (which is the minimal delay of all state items)
it wakes up, queries all state items the timeouts of which are expired,
and prints a lemonbar formatted string to stdout before going back to sleep.

Lemonbar updates every time a newline `\n` is read.

Also you can send SIGUSR1, which makes statorange force an update
(useful when adjusting the volume via command/button shortcuts).

In Lemonbar you can define button sections. When a button is pressed
a string is printed to stdout (coincidentally this string is a shell command)
which can and will be redirected to a shell.

To define other state items (like a VLC section which displays the currently playing song)
take a look at the existing items in `src/StateItems/`. The items are instantiated in
`src/StareItem.cpp:init_item()`. Include your .hpp, set the static settings, and add an if
to the loop below. Put the settings into the config.

## Unix socket notification\

State items can register sockets (file descriptors) which are listened to. When data arrives at a socket,
statorange will wake up (implemented using `select`) and notify the state item in question.
For an example on how this works see the `I3Workspaces` state item.

## Author

Alexander Dmitriev

Feel free to contact me about bugs or suggestions :)
