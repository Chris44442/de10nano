#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10

echo "Example:"
echo "mkdir -p sdcard && mount /dev/mmcblk0p1 ~/sdcard"
echo "./fpga_config_tool"
echo

ssh root@$IP
