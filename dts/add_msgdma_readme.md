# Add MSGDMA to use under Buildroot

See qsys for how to connect the ip.

use dtc to switch between dtb and dts.


```bash
dtc -I dts -O dtb -o abc.dtb abc.dts
```


change dts to include reserved memory at an address.

