#!/bin/bash
i=1
cellcenter=0
while [ $i -le 20 ]
do
	n=`cat su_100.txt | grep "Cell center" | cut -d'=' -f2 | head -$i | tail -1`
	echo $n
	cellcenter=`expr $cellcenter + $n`
	i=`expr $i + 1`
done
avgcellcenter=`expr $cellcenter / 20`
echo "Avg cell center = $avgcellcenter"
i=1
cre=0
while [ $i -le 20 ]
do
	n=`cat su_100.txt | grep "CRE RBs" | cut -d'=' -f2 | head -$i | tail -1`
	echo $n
	cre=`expr $cre + $n`
	i=`expr $i + 1`
done
avgcre=`expr $cre / 20`
echo "Avg CRE = $avgcre"


	


