#!/bin/bash
i=1;
while [ $i -le 50 ]
do
  ./waf --run scratch/csb_traffic_100_ca
  ./waf --run scratch/csb_traffic_200_ca
  ./waf --run scratch/csb_traffic_300_ca
  ./waf --run scratch/csb_traffic_400_ca
  ./waf --run scratch/csb_traffic_500_ca
  i=`expr $i + 1`
done
