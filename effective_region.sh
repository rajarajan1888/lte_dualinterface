#!/bin/bash
i=1;
while [ $i -le 100 ]
do
  ./waf --run scratch/csb_traffic_100
  ./waf --run scratch/csb_traffic_200
  ./waf --run scratch/csb_traffic_300
  ./waf --run scratch/csb_traffic_400
  ./waf --run scratch/csb_traffic_500
  i=`expr $i + 1`
done

