#!/bin/bash --login

cd `dirname "$0"`

CALL=`basename "$0"`
LINK=`readlink "$0"`

if [ -n "$LINK" -a $CALL != 'start.sh' ]; then
   DIR=`dirname "$LINK"`
   PRG=$DIR/$CALL
   CMD=$PRG" "$*
else
   DIR=`dirname "$0"`
   PRG=$DIR/$1
   CMD=$DIR/$*
fi

if [ "$CALL" = "fadctrl" ]; then
   NICE="ionice -c 2 -n 0"
fi

# echo DIR=$DIR
# echo PRG=$PRG
# echo CMD=$CMD

while [ true ]; do

   reset

   if [ -n "$RC" ]; then
      echo LAST RETURN CODE=$RC [`date -u`]
   fi

   echo COMMAND=$NICE $CMD
   echo

   if [ ! -x $PRG ]; then
      echo $1 not available... waiting 5s.
      sleep 5
      continue
   fi

   if [ -e $DIR/compiling.lock ]; then
      echo Compilation in progress... waiting 1s.
      sleep 1
      continue
   fi

   $NICE $CMD
   RC=$?

   echo RETURN CODE=$RC [`date -u`]
   echo

   # HUP    1  exit
   # INT    2  exit
   # QUIT   3  core
   # ILL    4  core
   # TRAP   5  core
   # ABRT   6  core
   # FPE    8  core
   # KILL   9  exit
   # SEGV  11  core
   # PIPE  13  exit
   # ALRM  14  exit
   # TERM  15  exit

   # 0    (User requested exit from the command line)
   # 1-   (Eval options failed, return code of StateMachineImp::Run)
   # 126  (shutdown requested)
   # 126  (exit requested from recompile.sh)
   # 127  (problem with option parsing or no program start, like --help)
   # 128  (exit(128))
   # 134  (ABRT, double corruption, abort())
   # 139  (SEGV, 128+11)
   # 255  (exception)

   if [ $RC -eq 126 ]; then
      continue
   fi

   # RC<=128 || RC==KILL || RC=TERM || RC=exception
   if [ $RC -le 128 ] || [ $RC -eq 137 ] || [ $RC -eq 143 ] || [ $RC -eq 255 ]; then
      exit
   fi

done
