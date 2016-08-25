#!/bin/bash
i=1;
while [ $i -le 50 ]
do
  ./waf --run scratch/abs_length_single_expt4_low
  i=`expr $i + 1`
done
i=1;
while [ $i -le 50 ]
do
  ./waf --run scratch/abs_length_single_expt2_low
  i=`expr $i + 1`
done
i=1;
while [ $i -le 50 ]
do
  ./waf --run scratch/abs_length_single_expt3_low
  i=`expr $i + 1`
done
i=1;
while [ $i -le 50 ]
do
  ./waf --run scratch/abs_length_single_low
  i=`expr $i + 1`
done
