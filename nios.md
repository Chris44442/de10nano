## Nios V

- Generate qsys in Platform Designer
- To build the bsp run:

```bash
niosv-shell -r "niosv-bsp -c -s=src/soc.sopcinfo -t=hal sw/bsp/settings.bsp"
```

- Generate the necessary build metadata and file structure to compile the application:
```bash
niosv-shell -r "niosv-app --bsp-dir=sw/bsp --app-dir=sw/app --srcs=sw/app/hello.c --elf-name=hello.elf"
```

- Set up the CMake build environment:

```bash
niosv-shell -r 'cmake -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE=Release -B sw/app/release -S sw/app'
```

- Run the makefile, compile the program:

```bash
niosv-shell -r "make -C sw/app/release"
```

- Hold the Nios V in reset, load program, and execute it:

```bash
niosv-shell -r "niosv-download sw/app/release/hello.elf -g -r"
```

- Open the JTAG UART terminal:


```bash
niosv-shell
```

... and then:

```bash
juart-terminal -c 1 -d 0 -i 0
```

## System Console

- in Platform Designer add JTAG to Avalon Master Bridge
- Connect a RAM e.g. at address 0x1000 to it
- After compiling open System Console and type:

```tcl
set masters [get_service_paths master]
set master [lindex $masters 0]
open_service master $master
master_write_32 $master 0x1000 0xaffe1234
set data [master_read_32 $master 0x1000 1]
puts $data
```

This should print 0xaffe1234.

