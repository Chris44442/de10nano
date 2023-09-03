#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10
RBF="../build/DE10.rbf"
RBF_HPS="~/sdcard/fpga.rbf"

start=`date +%s`

echo "HPS: Mounting SD card and copying rbf"

ssh root@$IP 'mkdir -p sdcard && mount /dev/mmcblk0p1 ~/sdcard'
scp $RBF root@$IP:$RBF_HPS
ssh root@$IP './fpga_config_tool'

# TimeStampInHex=`ssh root@$IP 'devmem 0xff200004' | tail -c 9`

# echo $TimeStampInHex

# date1=$(date -d @$(( 16#$TimeStampInHex )) '+%Y-%m-%d %H:%M')
# echo
# echo "Cyclone V 5CSEBA6U23I7 FPGA Image: " | GREP_COLOR="36" grep --color -P "Cyclone V 5CSEBA6U23I7 FPGA Image"
# echo "QSYS time:" $date1

end=`date +%s`
runtime=`expr $end - $start`
hours=`printf "%02d" $((runtime / 3600))`
minutes=`printf "%02d" $(( (runtime % 3600) / 60 ))`
seconds=`printf "%02d" $(( (runtime % 3600) % 60 ))`

echo -e "\nRuntime: $hours:$minutes:$seconds."
echo