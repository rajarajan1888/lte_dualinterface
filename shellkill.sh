#!/bin/bash
i=1;
killcnt=`ps aux | grep "scratch" | wc -l`;
killproc=1;
while [ $killproc -le $killcnt ]
do
	killjob=`ps aux | grep "scratch" | awk '{print $2}' | head -$killproc`;
	echo "Killing $killjob"
	kill -9 `ps aux | grep "scratch" | awk '{print $2}' | head -$killproc`;
	killproc=`expr $killproc + 1`;
done

