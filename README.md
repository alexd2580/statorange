# **Statorange**

|Build status|
|:--:|
|[![Unstable]]|

This is a status generator for i3 designed to replace i3bar/i3status.

## PREREQUISITES

Obviously, this software does NOT run on windows.
Statorange is tightly bound to the really awesome window manager i3,
so we have that. Also bar (lemonbar) is required for the actual panel.

The predefined status items use the following programs:
    The battery status depends on the [missing] file
        /sys/class/power_supply/BAT0/uevent
    and the ability to read files.
    
    Volume requires amixer to get the current volume
    and alsamixer, which opens when you click the volume section.
    
    Space requires "df -h" which _should_ be available from the get-go.
    
    The network item checks this file /sys/class/net/%s/operstate
    [replace %s by your network interface (eth0, wlan0)]
    to see whether the interface is up (or not).
    Also /sbin/ifconfig and /sbin/iwconfig are used to get info
    about the wired and wireless connection state.
    
    Date requires date.
    
    And the CPU temp/load is read from
    /sys/bus/acpi/devices/LNXTHERM:00/thermal_zone/temp and /proc/loadavg
    htop pops up when this section is clicked.
    
Also, since lemonbar does not provide a trayer section something like trayer
is advisable. You can add a shortcut to toggle_trayer.sh (i chose mod+t)
to toggle trayer, for when you really need it (you won't use them a lot, 
except for nm-applet or wicd to connect to a network GRAPHICALLY).

## INSTALLING STATORANGE

Basically... it's just typing make into the console. This compiles
the status generator (the part which produces a formatted string which
is fed to lemonbar).

## INSTALLING REQUIRED FONTS

The fonts used in statorange_launcher.sh have to be regustered.
./install.sh copies them to $HOME/.fonts. These commands must
be run on sartup (put them in your .xsession):

```shell
xset fp+ $HOME/.fonts/misc
xset fp+ $HOME/.fonts/ohsnap
xset fp+ $HOME/.fonts/terminesspowerline
xset fp+ $HOME/.fonts/tewi
```

## LAUNCHING STATORANGE

You can launch statorange with the statorange_launcher.sh script.
It does not take any parameters, it only produces logfiles.

```shell
lemonbar.log #from lemonbar
statorange.log #err stream from statorange
```

## INNER STRUCTURE

Statorange splits into two threads. The main one sleeps most of the time.
Every 5 seconds (default... probably) it wakes up, queries the WM state, 
updates the system state if necessary (you can set the update interval for each item),
and prints a lemonbar formatted string to stdout before going back to sleep.

Lemonbar updates every time a newline is read.

The second thread, the event listener, opens a second UNIX socket to the WM
and registers for some events (which affect the status bar, like workspace events).
When an event is received, the event listener processes it and
notifies the main thread, which printfs the formatted string.

Also you can send SIGUSR1, which makes statorange force an update
(useful when adjusting the volume via command/button shortcuts).

In Lemonbar you can define button sections. When a button is pressed
a string is printed to stdout (coincidentally this string is a shell command)
which can and will be redirected to a shell.
