#!/bin/bash
ctr=1
while [ $ctr -le 4 ]
do
	sum=0
	i=1
	while [ $i -le 50 ]
	do
		filter=`expr $ctr \* 50`	

		n=`cat sinrlogs_low.txt | grep "Total number of UEs" | cut -d'=' -f2 | head -$filter | tail -50 | head -$i | tail -1`
#		echo $n
		sum=`expr $sum + $n`
		i=`expr $i + 1`
	done
avg=`expr $sum / 50`;
echo "Total No. of RBs = $avg"
ctr=`expr $ctr + 1`
done

