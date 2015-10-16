#!/bin/sh

trayer_cmd="trayer --edge top --align right --widthtype pixel --width 120 --transparent true  --SetDockType true --heighttype pixel --height 20 --tint 0x000000 --alpha 0"

pid_of_trayer=`pidof trayer`
found_any=$?

if [ $found_any -eq 1 ]
then
  $trayer_cmd &
else
  kill $pid_of_trayer
fi
