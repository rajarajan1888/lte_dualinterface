#!/bin/bash
i=1
sum=0
while [ $i -le 20 ]
	do
		n=`cat perf_eval_0.txt | grep "Cell center" | tail -20 | cut -d';' -f2 | head -$i | tail -1`;
		echo $n;
		sum=`expr $sum + $n`;
		i=`expr $i + 1`;
	done
avg=`expr $sum / 20`;
echo "Total No. of RBs = $avg"
done

