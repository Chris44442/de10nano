#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10
RBF="../build/DE10.rbf"
RBF_HPS="~/sdcard/soc_system.rbf"

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

# Hardware=`ssh root@$IP 'memtool 0xc1000200 C'`
# CompileVersTime=`echo "$Hardware" | awk 'NR==3{print $5}'`
#
TimeStampInHex=`ssh root@$IP 'memtool 0xff200004 1' | tail -c 10`
date1=$(date -d @$(( 16#$TimeStampInHex )) '+%Y-%m-%d %H:%M')
echo
echo "Cyclone V 5CSEBA6U23I7 FPGA Image: " | GREP_COLOR="36" grep --color -P "Cyclone V 5CSEBA6U23I7 FPGA Image"
echo "QSYS time:" $date1
# echo -e "SYNIO_SA2: v${CompileVersTime:0:1}.0${CompileVersTime:1:1}.${CompileVersTime:2:2}"

end=`date +%s`
runtime=`expr $end - $start`
hours=`printf "%02d" $((runtime / 3600))`
minutes=`printf "%02d" $(( (runtime % 3600) / 60 ))`
seconds=`printf "%02d" $(( (runtime % 3600) % 60 ))`

echo -e "\nRuntime: $hours:$minutes:$seconds."
