#!/bin/bash
i=1
sum=0
while [ $i -le 20 ]
	do
		n=`cat abs_600.txt | grep "Normalized" | cut -d'=' -f3 | head -$i | tail -1`;
		echo $n;
		sum=`expr $sum + $n`;
		i=`expr $i + 1`;
	done
avg=`expr $sum / 20`;
echo "Total No. of RBs = $avg"


