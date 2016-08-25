#!/bin/bash
i=1;
while [ $i -le 50 ]
do
  ./waf --run scratch/abs_length_single
  i=`expr $i + 1`
done
