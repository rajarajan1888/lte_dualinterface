#!/bin/bash
i=1
creavg=0
creavg1=0
creavg2=0

totcreavg=0
totcreavg1=0
totcreavg2=0

while [ $i -le 20 ]
do
	n=`cat thr_600.txt | grep "Total throughput" | cut -d':' -f2 | head -$i | tail -1`

	n1=`cat thr_600.txt | grep "Total throughput 1" | cut -d':' -f2 | head -$i | tail -1`

	n2=`cat thr_600.txt | grep "Total throughput 2" | cut -d':' -f2 | head -$i | tail -1`

	cre=`cat thr_600.txt | grep "CRE UEs" | cut -d';' -f2 | head -$i | tail -1`

	avg=`expr $n / $cre`
	avg1=`expr $n1 / $cre`
	avg2=`expr $n2 / $cre`

	totcreavg=`expr $creavg + $avg`
	totcreavg1=`expr $creavg1 + $avg1`	
	totcreavg2=`expr $creavg2 + $avg2`		
	i=`expr $i + 1`
done
avgcreavg=`expr $totcreavg / 20`
avgcreavg1=`expr $totcreavg1 / 20`
avgcreavg2=`expr $totcreavg2 / 20`
echo "Avg cell center = $avgcreavg"
echo "Avg cell center 1 = $avgcreavg1"
echo "Avg cell center 2 = $avgcreavg2"


	


