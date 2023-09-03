#!/bin/bash

source ~/.fpga_config_de10
IP=$SOC_IP_DE10

CFGTOOL="../sw/fpga_config_tool/fpga_config_tool"
CFGTOOL_HPS="~/fpga_config_tool"

scp $CFGTOOL root@$IP:$CFGTOOL_HPS