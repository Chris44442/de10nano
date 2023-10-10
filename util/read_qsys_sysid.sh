#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10

QSYS_DATA=`ssh root@$IP 'devmem 0xff200000 64'`
QSYS_TIME=$(date -d @$(( 16#${QSYS_DATA:2:8} )) '+%Y-%m-%d %H:%M')
echo "DE10-Nano FPGA config meta info: " | GREP_COLOR="36" grep --color -P "DE10-Nano FPGA config meta info"
echo "QSYS time:" $QSYS_TIME
echo "QSYS ID: 0x"${QSYS_DATA:10:8}