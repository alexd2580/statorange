#ifndef __STATORANGE_CONFIG__
#define __STATORANGE_CONFIG__

#define getDate "date +%Y-%m-%d\\ %H:%M"
#define getSpace "df -h %s"

#define ifconfig_file_loc "/sbin/ifconfig"
#define iwconfig_file_loc "/sbin/iwconfig"
#define shell_file_loc "/bin/sh"
#define temp_file_loc "/sys/bus/acpi/devices/LNXTHERM:00/thermal_zone/temp"
#define load_file_loc "/proc/loadavg"
#define bat_file_loc "/sys/class/power_supply/BAT0/uevent"
#define ifstat_file_loc "/sys/class/net/%s/operstate"

#define ethernet "eth0"
#define wireless "wlan0"

#define home_dir "/home"
#define root_dir "/"

#endif
