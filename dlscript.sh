#!/bin/bash
i=1
count=`cat DlRsrpSinrStatsHetNet.txt | wc -l`
while [ $i -le  $count ]
do
	cat DlRsrpSinrStatsHetNet.txt | awk '{print $2,$3,$6}' | head -$i | tail -1 >> DlSinr.txt
	i=`expr $i + 4`
done
