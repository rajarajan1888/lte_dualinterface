#!/bin/bash
echo "Total arguments is $#"
entries=`wc -l $1`
echo "Total entries is $entries"
totachthr=0.0
i=1
userstats=0;
while [ $i -le `expr 10 \* 317` ]
do
	thr=`cat $1 | cut -d"," -f2 | head -$i | tail -1`
	user=`cat $2 | cut -d"," -f2 | head -$i | tail -1`

	if [ $user == 1 ]
	then
		userstats=`expr $userstats + 1`
	fi
	thrval=`expr $thr \* $user`
	totachthr=`expr $totachthr + $thrval`
	i=`expr $i + 1`
	echo "$i,$thr"
done
avgthr=`expr $totachthr / 10`;
userstats=`expr $userstats / 10`;
echo "Tot achievable throughput (cell center) is $totachthr"
echo "Average achievable throughput (cell center) is $avgthr"
echo "Average no. of users in the inner circle is $userstats"
