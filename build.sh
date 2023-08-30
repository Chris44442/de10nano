#!/bin/bash
QUARTUS_COMPILE_DIR_DE10=""
source ~/.fpga_config_de10

start=`date +%s`

cd "src"
eval ${QUARTUS_COMPILE_DIR_DE10}quartus_sh --flow compile "DE10"
cd ..

end=`date +%s`
runtime=`expr $end - $start`
hrs=`printf "%02d" $((runtime / 3600))`
min=`printf "%02d" $(( (runtime % 3600) / 60 ))`
sec=`printf "%02d" $(( (runtime % 3600) % 60 ))`

echo -e "\nRuntime: $hrs:$min:$sec"
