#!/bin/bash
i=1
while [ $i -le 20 ]
do
	./waf --run throughput_600
	i=`expr $i + 1`
done


	


