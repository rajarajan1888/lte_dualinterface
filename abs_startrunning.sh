#!/bin/bash
i=1
while [ $i -le 20 ]
do
	./waf --run scratch/abs_length_single
	i=`expr $i + 1`
done
i=1
while [ $i -le 20 ]
do
	./waf --run scratch/abs_length_single_expt2
	i=`expr $i + 1`
done
i=1
while [ $i -le 20 ]
do
	./waf --run scratch/abs_length_single_expt3
	i=`expr $i + 1`
done
i=1
while [ $i -le 20 ]
do
	./waf --run scratch/abs_length_single_expt4
	i=`expr $i + 1`
done
i=1
while [ $i -le 20 ]
do
	./waf --run scratch/abs_length_single_expt5
	i=`expr $i + 1`
done
i=1
while [ $i -le 20 ]
do
	./waf --run scratch/abs_length_single_expt6
	i=`expr $i + 1`
done




