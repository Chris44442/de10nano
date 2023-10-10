#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10
MEM="target/arm-unknown-linux-gnueabi/debug/memtool3"
MEM_HPS="~/memtool3"

cargo build --target=arm-unknown-linux-gnueabi

scp $MEM root@$IP:$MEM_HPS
ssh root@$IP './memtool3'
