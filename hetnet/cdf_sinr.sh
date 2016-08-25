#!/bin/bash
i=1;
while [ $i -le 200 ]
do
  j=`expr $i \* 2`
  cat DlMacStatsatt.txt | head -$j | tail -1 | awk '{print $3,$6}'
  i=`expr $i + 1`
done

