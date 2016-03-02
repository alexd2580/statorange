#!/bin/bash

this_path=`dirname "$0"`
log_path=$this_path"/log"
mkdir -p $log_path
exit_value=4
echo `date`

normal_font="-f -misc-fixed-medium-r-normal--13-120-75-75-C-70-iso10646-1"
powerline_font="-f -xos4-terminesspowerline-bold-r-normal--12-120-72-72-c-60-iso10646-1"
powerline_icons="-f -xos4-terminusicons2mono-medium-r-normal--12-120-72-72-m-60-iso8859-1"

#transparency_cmd=xcompmgr
lemonbar_cmd="lemonbar $normal_font $powerline_font $powerline_icons -u -1 -a 30"
statorange_cmd=$this_path"/statorange"
statorange_config=$this_path"/config.json"
statorange_fifo=$this_path"/statorange.fifo"

lemonbar_log=$log_path"/lemonbar.log"
statorange_log=$log_path"/statorange.log"
command_log=$log_path"/command.log"

rm -f $statorange_log $lemonbar_log
echo `date` > $statorange_log
echo `date` > $lemonbar_log

rm -f $statorange_fifo
mkfifo $statorange_fifo

while [ $exit_value -eq 4 ]
do
  #echo -n "Launching transparency manager ... "
  #$transparency_cmd &
  #transparency_pid=$!
  #echo $transparency_pid

  socket_path=`i3 --get-socketpath`
  echo "Socket path: $socket_path"

  echo -n "Launching statorange ... "
  $statorange_cmd $socket_path $statorange_config 2>> $statorange_log 1> $statorange_fifo &
  statorange_pid=$!
  echo $statorange_pid

  echo -n "Launching lemonbar ... "
  cat $statorange_fifo | $lemonbar_cmd 2>> $lemonbar_log | $SHELL -v 2>> $command_log &
  echo $!

  wait $statorange_pid
  exit_value=$?
  dt=`date`
  echo "[$dt] $statorange_cmd terminated with exit value $exit_value"
  #kill $transparency_pid
done

rm -f $statorange_fifo

dt=`date`
echo "[$dt] statorange_launcher.sh exiting"
