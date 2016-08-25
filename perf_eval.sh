#!/bin/bash
i=1;
while [ $i -le 20 ]
do
  ./waf --run scratch/perf_eval_0
  i=`expr $i + 1`
done
i=1;
while [ $i -le 20 ]
do
  ./waf --run scratch/perf_eval_1
  i=`expr $i + 1`
done
i=1;
while [ $i -le 20 ]
do
  ./waf --run scratch/perf_eval_2
  i=`expr $i + 1`
done
i=1;
while [ $i -le 20 ]
do
  ./waf --run scratch/perf_eval_3
  i=`expr $i + 1`
done
while [ $i -le 20 ]
do
  ./waf --run scratch/perf_eval_4
  i=`expr $i + 1`
done
i=1;
while [ $i -le 20 ]
do
  ./waf --run scratch/perf_eval_5
  i=`expr $i + 1`
done
i=1;
while [ $i -le 20 ]
do
  ./waf --run scratch/perf_eval_6
  i=`expr $i + 1`
done
i=1;
while [ $i -le 20 ]
do
  ./waf --run scratch/perf_eval_7
  i=`expr $i + 1`
done
