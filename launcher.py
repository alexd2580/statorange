#!/usr/bin/python3

import os
import sys
import time
import subprocess
from datetime import datetime

this_path = sys.path[0]
log_path = this_path + '/log'

os.system('mkdir -p ' + log_path)
exit_value = 4

os.system('date')

normal_font = '-misc-fixed-medium-r-normal--13-120-75-75-C-70-iso10646-1'
powerline_font = '-xos4-terminesspowerline-bold-r-normal--12-120-72-72-c-60-iso10646-1'
powerline_icons = '-xos4-terminusicons2mono-medium-r-normal--12-120-72-72-m-60-iso8859-1'

lemonbar_cmd = [
    'lemonbar',
    '-f',
    normal_font,
    '-f',
     powerline_font,
     '-f',
     powerline_icons,
     '-u',
     '-1',
     '-a',
     '30']
statorange_cmd = this_path + '/statorange_debug'
statorange_config = this_path + '/config.json'

lemonbar_log = log_path + '/lemonbar.log'
command_log = log_path + '/command.log'

os.system('rm -f ' + statorange_log + ' ' + lemonbar_log)

while exit_value == 4:
    (socket_path, _) = subprocess.Popen(['i3', '--get-socketpath'], stdout=subprocess.PIPE).communicate()
    socket_path = socket_path[:-1]
    print('Socket path:', socket_path)

    print('Launching statorange', end=' ')
    statorange = subprocess.Popen(
        [statorange_cmd, socket_path, statorange_config],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    print(statorange.pid)

    print('Launching lemonbar')
    lemonbar = subprocess.Popen(
        lemonbar_cmd,
        stdin=statorange.stdout)

    statorange.wait()
    exit_value = statorange.returncode
    os.system('date')
    print(statorange_cmd, 'terminated with exit value', exit_value)
    lemonbar.terminate()

os.system('date')
print('launcher.sh exiting')
