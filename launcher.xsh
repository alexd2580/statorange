#!/usr/bin/xonsh

import os
import sys
import xonsh


"""
The $(<expr>) operator in xonsh executes a subprocess
command and captures some information about that command.

The $() syntax captures and returns the standard output stream of the command
The !() syntax captured more information about the command,
as an instance of a class called CompletedCommand.
This object contains more information about the result of the given command,
including the return code, the process id, the standard output and standard
error streams, and information about how input and output were redirected.

@(<expr>) operator form works in subprocess mode,
and will evaluate arbitrary Python code.
"""

this_path = $(dirname "$0")[:-1]
log_path = this_path + '/log'
log_path_old = log_path + '_old'
exit_value = 4

print($(date), end='')

normal_font = '-f-misc-fixed-medium-r-normal--13-120-75-75-C-70-iso10646-1'
powerline_font = '-f-xos4-terminesspowerline-bold-r-normal--12-120-72-72-c-60-iso10646-1'
powerline_icons = '-f-xos4-terminusicons2mono-medium-r-normal--12-120-72-72-m-60-iso8859-1'

#transparency_cmd=xcompmgr
lemonbar_cmd = ['lemonbar', normal_font, powerline_font, powerline_icons, '-u', '-1', '-a', '30']
statorange_bin = this_path + '/statorange'
statorange_config = this_path + '/config.json'
statorange_fifo = this_path + '/statorange.fifo'
shell_bin = '/usr/bin/zsh'
shell_cmd = [shell_bin, '-v']

lemonbar_log = log_path + '/lemonbar.log'
statorange_log = log_path + '/statorange.log'
command_log = log_path + '/command.log'

rm -rf @(log_path_old)
if os.path.exists(log_path):
    mv @(log_path) @(log_path_old)
mkdir -p @(log_path)

rm -f @(statorange_log) @(lemonbar_log) @(command_log)
echo $(date) > @(statorange_log)
echo $(date) > @(lemonbar_log)

rm -f @(statorange_fifo)
mkfifo @(statorange_fifo)

try:
    while exit_value == 4:
        #echo -n "Launching transparency manager ... "
        #$transparency_cmd &
        #transparency_pid=$!
        #echo $transparency_pid

        socket_path = $(i3 --get-socketpath)[:-1]
        print('Socket path: ', socket_path)
        statorange_cmd = [statorange_bin, socket_path, statorange_config]

        print('Launching lemonbar.')
        next_jobnum = xonsh.jobs.get_next_job_number()
        cat @(statorange_fifo) | @(lemonbar_cmd) 2>> @(lemonbar_log) | @(shell_cmd) all>> @(command_log) &
        lemonbar_job = __xonsh_all_jobs__[next_jobnum]
        print('lemonbar pid: ', lemonbar_job['pgrp'])

        print('Launching statorange.')
        next_jobnum = xonsh.jobs.get_next_job_number()
        @(statorange_cmd) 1> @(statorange_fifo) 2>> @(statorange_log) &
        statorange_job = __xonsh_all_jobs__[next_jobnum]
        print('statorange pid: ', statorange_job['pgrp'])

        exit_value = statorange_job['obj'].wait()
        print('[', $(date)[:-1], end=' ] ')
        print('{} terminated with exit value {}'.format(statorange_bin, exit_value))

except:
    print('An exception occured, shutting down.')

    if not lemonbar_job is None:
        for pid in lemonbar_job['pids']:
            kill -SIGKILL @(pid)

    if not statorange_job is None:
        for pid in statorange_job['pids']:
            kill -SIGKILL @(pid)

rm -f @(statorange_fifo)

#print('[{}] launcher.xsh exiting'.format($(date)))
