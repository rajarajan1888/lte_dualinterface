#!/bin/bash
i=1
while [ $i -le 10 ]
do
	echo "Iteration $i"
	./waf --run scratch/infocom_test_multi
	i=`expr $i + 1`
done

