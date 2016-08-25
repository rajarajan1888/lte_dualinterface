#!/bin/bash
i=1;
while [ $i -le 10 ]
do
  ./waf --run scratch/blankingtest_600
  i=`expr $i + 1`
done

