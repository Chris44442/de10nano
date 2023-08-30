#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10

echo "Examples:"
echo "=========================================="
echo "connmanctl services"
echo "netstat -rn"
echo "connmanctl config ethernet_000000000000_cable --ipv4 manual $IP 255.255.0.0 0.0.0.0"
echo "findmnt"
echo "memtool 0xff200000 40"
echo "mount /dev/mmcblk0p1 ~/sdcard"
echo "lsblk"
echo "cat /etc/fstab"
echo "=========================================="

ssh root@$IP
