#!/bin/bash
i=1
while [ $i -le 20 ]
do
	./waf --run throughput_200
	i=`expr $i + 1`
done


	


