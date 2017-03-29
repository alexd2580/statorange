#!/bin/bash

echo Starting statorange

this_path=`dirname "$0"`
echo Working directory: $this_path
log_path=$this_path"/log"
echo Log file directory: $log_path

mkdir -p $log_path

# Define fonts.
normal_font="-f -misc-fixed-medium-r-normal--13-120-75-75-C-70-iso10646-1"
powerline_font="-f -xos4-terminesspowerline-bold-r-normal--12-120-72-72-c-60-iso10646-1"
powerline_icons="-f -xos4-terminusicons2mono-medium-r-normal--12-120-72-72-m-60-iso8859-1"

# Define commands and file paths.
lemonbar_cmd="lemonbar $normal_font $powerline_font $powerline_icons -u -1 -a 30"
statorange_cmd=$this_path"/statorange_debug"
statorange_config=$this_path"/config.json"
statorange_fifo=$this_path"/statorange.fifo"
shell_cmd="/bin/bash"
lemonbar_log=$log_path"/lemonbar.log"
command_log=$log_path"/command.log"

rm -f $lemonbar_log $command_log $statorange_fifo

socket_path=`i3 --get-socketpath`
echo Socket path: $socket_path

mkfifo $statorange_fifo

echo Launching statorange...
echo "$statorange_cmd $socket_path $statorange_config 1> $statorange_fifo &"
if [ "$1" == "dry" ]; then exit; fi
$statorange_cmd $socket_path $statorange_config 1> $statorange_fifo &
statorange_pid=$!
echo PID: $statorange_pid

echo Launching lemonbar...
echo "cat $statorange_fifo | $lemonbar_cmd 2>> $lemonbar_log | $shell_cmd -v 2>> $command_log &"
cat $statorange_fifo | $lemonbar_cmd 2>> $lemonbar_log | $shell_cmd -v 2>> $command_log &
lemonbar_pid=$!
echo PID: $lemonbar_pid

wait $statorange_pid
exit_value=$?
echo $statorange_cmd terminated with exit value $exit_value

rm -f $statorange_fifo

echo launcher.sh exiting
