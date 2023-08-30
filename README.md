# DE10-Nano design for rapid prototyping

This repository contains tools to build and test Firmware for the Terasic DE10-Nano.

## Dependencies

Supported Operating Systems:

- Ubuntu 22.04 (other distros might work too)
- Windows 10

Install the following packages on the system:

- Quartus 22.1std for compilation (other std and lite versions might work too)

You will also need:

- The DE10-Nano board
- A prepared SD card with the Linux Console (kernel 4.5) v1.3 (2018-03-15) image from Terasic
- Micro USB and Ethernet cable for communication, power cable

## Files and Folders

- **build**
  Generated folder for the built artefacts e.g. bitstream files

- **doc**
  Documentation materials

- **src**
  Source code for building the design

- **sw**
  Software examples that can be run on the HPS

- **test**
  Testbench files and scripts for simulation tests of the Firmware via QuestaSim

- **util**
  Utilities and hardware scripts, e.g. for programming the device

- **build.sh**
  Bash script to build the design via Quartus

- **readme.md**
  This readme file

## Setup

You need to have the config file `.fpga_config_de10` present in your `~` home directory. It must contain the following (adjust to your machine):

```
QUARTUS_COMPILE_DIR_DE10="~/intelFPGA/22.1std/quartus/bin/"
SOC_IP_DE10="169.254.42.42"
SOC_SUBNETMASK_DE10="255.255.0.0"
```

Clone or download this repository.

## Build the Design

Run the `build.sh` script to build the design. It will generate QSYS and IP files, synthezise, place and route the complete design and build the desired artefacts.

You can clean up generated files with `git clean -fdx`.

## Remote System Updates

TODO

## Access the FPGA logic via HPS

On Angstrom Linux you can utilize the `memtool` command to access the entire HPS address space, including the FPGA Slaves (AXI), the FPGA Lightweight (AXI Lite), the common SDRAM and more.
Refer to the [Cyclone V HPS Technical Reference Manual](https://www.intel.com/content/www/us/en/docs/programmable/683126/21-2/hard-processor-system-technical-reference.html). Refer to `src/soc.qsys` in Quartus for FPGA slave base addresses.

For a quick example on how to remotely access some FPGA status registers, run `util/ReadImageMetaInfo.sh`.
