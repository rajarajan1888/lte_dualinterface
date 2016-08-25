#!/bin/bash
i=20
usedup=0
while [ $i -ge 1 ]
do
	n=`cat abs_300.txt | grep "Normalized" | cut -d'=' -f2 | tail -$i | head -1`
	echo $n
	usedup=`expr $usedup + $n`
	i=`expr $i - 1`
done
avgusedup=`expr $usedup / 20`
echo "Avg used up = $avgusedup"
i=10
compromisedrbs=0
while [ $i -ge 1 ]
do
	n=`cat compromise_300.txt | grep "to macro" | cut -d':' -f2 | tail -$i | head -1`
	i=`expr $i - 1`
	compromisedrbs=`expr $compromisedrbs + $n`
	echo $n
done
avgcompromised=`expr $compromisedrbs / 10`
echo "Avg compromised = $avgcompromised"
usedup=${avgusedup%.*}
compromised=${avgcompromised%.*}
rbstotal=`expr $usedup + $compromised`
echo "Total RBs = $rbstotal"

