i=1
while [ $i -le `expr 10 \* 317` ]
do
	echo "Iteration $i"
	bash getcenterstats.sh specefflogs.txt rsrplogs.txt
	i=`expr $i + 1`
done
