#!/bin/bash

if [ ! -f compiling.lock ]; then

   touch compiling.lock

   if [ ! -x dimctrl ]; then
      make $* dimctrl || exit
   fi

   dimctrl --quit --cmd ".w 3000" --cmd "DIS_DNS/KILL_SERVERS 126" --cmd ".w 3000" --cmd "DIS_DNS/EXIT 126"

   sleep 5

   make clean
fi

make $* && rm compiling.lock && sleep 15 && dimctrl --quit --cmd '.js scripts/check.js'
