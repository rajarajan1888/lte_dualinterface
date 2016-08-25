#!/bin/bash
i=1
usedup=0
while [ $i -le 20 ]
do
	n=`cat abs_200.txt | grep "Normalized" | cut -d'=' -f2 | head -$i | tail -1`
	echo $n
	usedup=`expr $usedup + $n`
	i=`expr $i + 1`
done
avgusedup=`expr $usedup / 20`
echo "Avg used up = $avgusedup"


