#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10
RBF="../build/DE10.rbf"
RBF_HPS="~/sdcard/fpga.rbf"

echo "HPS: Mounting SD card and copying rbf"

ssh root@$IP 'mkdir -p sdcard && mount /dev/mmcblk0p1 ~/sdcard'
scp $RBF root@$IP:$RBF_HPS
ssh root@$IP './fpga_config_tool'

QSYS_ID=`ssh root@$IP 'devmem 0xff200000'`
TimeStampInHex=`ssh root@$IP 'devmem 0xff200004' | tail -c 9`
date1=$(date -d @$(( 16#$TimeStampInHex )) '+%Y-%m-%d %H:%M')
echo
echo "DE10-Nano FPGA config meta info: " | GREP_COLOR="36" grep --color -P "DE10-Nano FPGA config meta info"
echo "QSYS time:" $date1
echo "QSYS ID:" $QSYS_ID