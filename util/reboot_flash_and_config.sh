#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10
RBF="../build/DE10.rbf"
RBF_HPS="~/sdcard/fpga.rbf"

start=`date +%s`

echo "HPS: Mounting SD card and copying rbf"

ssh root@$IP 'mkdir -p sdcard && mount /dev/mmcblk0p1 ~/sdcard'
scp $RBF root@$IP:$RBF_HPS
ssh root@$IP 'reboot'

echo "Rebooting HPS..."

until ssh root@$IP 'echo "Reboot done"' 2> /dev/null
do
sleep 1
done

QSYS_ID=`ssh root@$IP 'devmem 0xff200000'`
TimeStampInHex=`ssh root@$IP 'devmem 0xff200004' | tail -c 9`
date1=$(date -d @$(( 16#$TimeStampInHex )) '+%Y-%m-%d %H:%M')
echo
echo "DE10-Nano FPGA config meta info: " | GREP_COLOR="36" grep --color -P "DE10-Nano FPGA config meta info"
echo "QSYS time:" $date1
echo "QSYS ID:" $QSYS_ID

end=`date +%s`
runtime=`expr $end - $start`
hours=`printf "%02d" $((runtime / 3600))`
minutes=`printf "%02d" $(( (runtime % 3600) / 60 ))`
seconds=`printf "%02d" $(( (runtime % 3600) % 60 ))`

echo -e "\nRuntime: $hours:$minutes:$seconds."