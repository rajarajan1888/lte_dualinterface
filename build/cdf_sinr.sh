#!/bin/bash
i=1;
while [ $i -le 20 ]
do
  j=`expr $i \* 4`
  cat DlRsrpSinrStatsHetNet.txt | head -j | tail -1 | awk '{print $6}'
  i=`expr $i + 1`
done

