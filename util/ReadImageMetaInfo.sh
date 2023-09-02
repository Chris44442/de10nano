#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10

# TimeStampInHex=`ssh root@$IP 'memtool 0xff200004 1' | tail -c 10`
# date1=$(date -d @$(( 16#$TimeStampInHex )) '+%Y-%m-%d %H:%M')
#
# echo
# echo "Cyclone V 5CSEBA6U23I7 FPGA Image: " | GREP_COLOR="36" grep --color -P "Cyclone V 5CSEBA6U23I7 FPGA Image"
# echo "QSYS time:" $date1
# echo

echo
echo TODO
echo