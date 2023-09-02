# DE10-Nano Cyclone V SoC example design for rapid prototyping

FPGA designs are notorious for being slow to deploy. Utilizing the power of embedded Linux, SD card flash memory, the HPS FPGA Manager, (and optional High Level Synthesis tools) we can drastically reduce the time to deploy and test new FPGA designs. This time, mainly due to its popularity and amiable pricetag, the target of choice is the Terasic DE10-Nano Cyclone V SoC board. This repo provides the necessary sources and tools to build a system on which you can rapidly prototype and test new FPGA designs.

## Dependencies

You will need:
- Host PC with any Linux distro that can run Quartus (tested on Ubuntu 22.04), Windows 10 might work too
- Quartus std or Lite for compilation (tested with 22.1)

For the target hardware you need:
- The DE10-Nano board itself
- 5V power cable
- Micro SD card with at least 1GB
- USB Mini-B cable for UART communication between Host PC and HPS
- LAN cable for Ethernet communication between Host PC and HPS

## Files and Folders

- **build** Generated folder for the built artefacts e.g. bitstream files
- **doc** Documentation materials
- **src** Source code
- **sw** Software that can be run on the ARM, or on Softcores if applicable
- **test** Files for rtl simulation and hardware tests on target platform
- **util** Utilities and hardware scripts, e.g. for flashing and configuring the FPGA
- **build.sh** Bash script to build the FPGA/HPS design via Quartus

## Build the FPGA Design and the HPS

Clone or download this repository. The build and util scripts rely on you having the config file `.fpga_config_de10` present in your `~` home directory. It must contain the following (replace with your values):

```
QUARTUS_COMPILE_DIR_DE10="~/intelFPGA/22.1std/quartus/bin/"
SOC_IP_DE10="169.254.42.42"
```

Run the `build.sh` script to build the design. It will generate QSYS and IP files, synthezise, place and route the FPGA design and build the rbf-file. If you're on Quartus Lite you might need to build via the GUI since script support is limited. In Lite it might also be necessary to convert the sof to rbf manually in the Convert Programming File GUI, be sure to set it to passive parallel x16 and enable compression.

You can clean up generated files with `git clean -fdx`.

## Build the FPGA Config Tool

On a SoC the u-boot or OS can access the FPGA Manager in order to configure the FPGA. The FPGA Config Tool runs on any embedded Linux like Angstrom or Buildroot.

This tool was developed by [Nicolás Hasbún](https://github.com/nhasbun/de10nano_fpga_linux_config). Due to minor changes, there is a copy in this repo as well. It is fully implemented in C using direct register access to configure the FPGA via HPS FPGA Manager. Partial reconfiguration might be worth studying in the future.

Get the appropriate cross compiler with:

```
sudo apt install gcc-arm-linux-gnueabi
```

In `sw/fpga_config_tool` run `make` to build `fpga_rbf_load`. This will later be used to configure the FPGA from the OS.

If you want to use this tool on other boards, you might need to change the line `char rbf_file [32] = "sdcard/fpga.rbf";` and also `  uint8_t  cdratio      = 0x3;` in the `main.c`. It might also be necessary to set a different target for cross compilation, e.g. `CROSS_COMPILE = arm-linux-gnueabihf-` in the `makefile`.

## Build Buildroot

Understanding how to create an SD card that the HPS can boot Linux from scratch, can be quite daunting, especially for beginners. This Chapter 

These instructions show how to build an SD card with the mainline Linux Kernel, U-Boot, and the Buildroot root file system.

### Prerequisites

A Ubuntu 22.04 system was used for this build, but most Linux systems should work fine.

```
# For building U-Boot and the Linux kernel
sudo apt install libncurses-dev flex bison openssl \
        libssl-dev dkms libelf-dev libudev-dev libpci-dev \
        libiberty-dev autoconf bc libmpc-dev libgmp-dev
```

Create a directory to build everything in.

```
mkdir -p ~/de10
cd ~/de10
```
### Get an ARM toolchain

Download the latest stable armebv7-eabihf / glibc toolchain from bootlin.

```
# cd to de10 directory
mkdir toolchain
cd toolchain
wget https://toolchains.bootlin.com/downloads/releases/toolchains/armv7-eabihf/tarballs/armv7-eabihf--glibc--stable-2020.08-1.tar.bz2
tar -xf armv7-eabihf--glibc--stable-2020.08-1.tar.bz2
rm armv7-eabihf--glibc--stable-2020.08-1.tar.bz2
# The CROSS_COMPILE variable must be set in any terminal used to build u-boot or the kernel
export CROSS_COMPILE=$PWD/armv7-eabihf--glibc--stable-2020.08-1/bin/arm-linux-
```

### Build the u-boot bootloader

Some versions of U-boot don't seem to work properly on the DE10-Nano, so it is recommended to stick with the version stated below.

Make sure CROSS_COMPILE variable is still set in the environment.

```
# cd to de10 directory
git clone https://github.com/u-boot/u-boot.git
cd u-boot
# This is the latest version at time of writing
git checkout v2021.07
# Configure the the DE10 nano Intel FPGA SoC
make ARCH=arm socfpga_de10_nano_defconfig
# Optional: run make ARCH=arm menuconfig to have a play around
make ARCH=arm -j $(nproc)
# Desired output file is: u-boot/u-boot-with-spl.sfp
```

The defconfig for DE10-nano is in `u-boot/configs/socfpga_de10_nano_defconfig`. When this config is set, `u-boot/arch/arm/mach-socfpga/Kconfig` sets the device to: `u-boot/board/terasic/de10-nano/`. In `u-boot/board/terasic/de10-nano/qts` (qts = Quartus) there are 4 files that set ram timing, pin MUXing, and PLL settings. The defaults will work for most applications, but if required they can be changed manually, or can be generated from the BSP handover files generated by the Intel Quartus software. See `u-boot/docs/README.socfpga` for more info.


### Build the Linux kernel

Make sure CROSS_COMPILE variable is still set in the environment.

```
# cd to de10 directory
git clone https://github.com/torvalds/linux.git
cd linux
# This is the latest version at time of writing
git checkout v6.5
# Configure kernel
make ARCH=arm socfpga_defconfig
make ARCH=arm menuconfig
```

Set the following kernel options:

- Under General setup and uncheck "Automatically append version information to the version string" (optional).
- Under File systems, enable "Overlay Filesystem Support" and all the options under it.
- Under File systems, Pseudo File Systems, Enable "Userspace-driven configuration filesystem".

Build the kernel:

```
# Compile kernel - This will take several minutes
make ARCH=arm LOCALVERSION=zImage -j $(nproc)
# Desired output file is: linux/arch/arm/boot/zImage

# Optionally save the config for future use
make savedefconfig
mv defconfig arch/arm/configs/socfpga_de10_defconfig
```

### Make a Buildroot root filesystem

This guide leads through the rootfile system generation.

- Create a new folder in a known existing folder via mkdir rootfs
- Change into the new folder via cd rootfs
- Clone the git repository via git clone https://github.com/buildroot/buildroot
- Change into the new folder via cd buildroot
- Look into the branch overview inside the repository via git branch -a
- Select one of the branches and check it out via git checkout <selection>
- Open the buildroot configuration dialogue via make nconfig
- Configure the buildroot in this window:
  - Target options → Target Architecture → ARM (little endian)
  - Target options → Target Architecture Variant → cortex-A9
  - Target options → Target ABI → EABI
  - Target options → Enable NEON SIMD extension support
  - Target options → Floating point strategy → NEON
  - Toolchain → Toolchain type → Buildroot toolchain
  - System Configuration → System hostname → <Select a name>
  - System Configuration → System banner → <Select a system banner>
  - System Configuration → Init System → BusyBox
  - System Configuration → /dev management → Dynamic using devtmpfs only
  - System Configuration → Enable root login with password
  - System Configuration → Root password → root
  - System Configuration →Enable Run a getty (login prompt) after boot
  - Filesystem images → Enable tar the root filesystem
  - Remember to also install desired compents such as nano, openssh, compilers, etc.
- Save the configuration via F6 → Enter → Enter
- Exit the configuration menu via F9
- Configure BusyBox if desired via make busybox-menuconfig
- Generate the root filesystem via make all
- After compilation, which can can last a longer time, the root filesystem (rootfs.tar) should be in folder buildroot/output/images/

### Create the SD card image

Create a blank image file: Create at least a 1 GB image, and connect it as a loop device.

```
# cd to de10 directory
sudo dd if=/dev/zero of=de10_nano_sd.img bs=1G count=1
sudo losetup --show -f de10_nano_sd.img
# Make sure you note the name of the loop device that is used.
# Typically: /dev/loopX where X is a number.
```

Partition the image

```
sudo fdisk /dev/loopX
```

Make the U-Boot and SPL partition:

```
n <Enter>, p <Enter>, 3 <Enter>, <Enter>, +1M <Enter>, t <Enter>, a2 <Enter>
```

Make the kernel partition:

```
n <Enter>, p <Enter>, 1 <Enter>, <Enter>, +254M <Enter>, t <Enter>, 1 <Enter>, b <Enter>
```

Make the Root Filesystem partition:

```
n <Enter>, p <Enter>, 2 <Enter>, <Enter>, <Enter>
```

Check partitions:

```
p <enter>
```

Expected output:

```
Device       Boot  Start     End Sectors  Size Id Type
/dev/loop0p1        4096  524287  520192  254M  b W95 FAT32
/dev/loop0p2      524288 2097151 1572864  768M 83 Linux
/dev/loop0p3        2048    4095    2048    1M a2 unknown
```

Write the partitions

```
w <Enter>
```

NB: The following command can instead be used to partition the disk in a scriptable (non-interactive) manner:

```
(
echo n; echo p; echo 3; echo; echo +1M;   echo t; echo a2; # Make the U-Boot partition
echo n; echo p; echo 1; echo; echo +254M; echo t; echo 1; echo b;  # Make kernel partition
echo n; echo p; echo 2; echo; echo; # Make the root file system partition
echo w; # Write the changes
) | sudo fdisk /dev/loopX
```

It is normal to get error "Re-reading the partition table failed.: Invalid argument".

Load the partitions:

```
sudo partprobe /dev/loopX
```

ls /dev/loopX* should show:

```
/dev/loopX  /dev/loopXp1  /dev/loopXp2  /dev/loopXp3
```

Create the filesystems

```
sudo mkfs -t vfat /dev/loopXp1
sudo mkfs -t ext4 /dev/loopXp2
```

Copy over U-BOOT. Remember to change /dev/loopX the the right value.

```
# cd to de10 directory
sudo dd if=./u-boot/u-boot-with-spl.sfp of=/dev/loopXp3 bs=64k seek=0 oflag=sync
```

Copy Kernel and device tree

```
# cd to de10 directory
mkdir fat
sudo mount /dev/loopXp1 fat
sudo cp linux/arch/arm/boot/zImage fat
# Use DE0 DTB for now as no DE10 DTB and they are basically the same board
sudo cp linux/arch/arm/boot/dts/socfpga_cyclone5_de0_nano_soc.dtb fat
```

Generate extlinux file


```
# cd to de10 directory
echo "LABEL Linux Default" > extlinux.conf
echo "    KERNEL ../zImage" >> extlinux.conf
echo "    FDT ../socfpga_cyclone5_de0_nano_soc.dtb" >> extlinux.conf
echo "    APPEND root=/dev/mmcblk0p2 rw rootwait earlyprintk console=ttyS0,115200n8" >> extlinux.conf
sudo mkdir -p fat/extlinux
sudo cp extlinux.conf fat/extlinux
sudo umount fat
rm extlinux.conf
```

Copy root filesystem. If you need to make changes to it, now is a good time. You might need to adjust the folder for rootfs.tar.

```
# cd to de10 directory
mkdir ext4
sudo mount /dev/loopXp2 ext4
cd ext4
sudo tar -xf ../rootfs.tar.bz2
# Useful changes can include setting up eth0 in /etc/network/interfaces with
# auto eth0
# iface eth0 inet static
#   address 169.254.x.x # x can be your choice
#   netmask 255.255.0.0
#   network 0.0.0.0
#
# And also setting up SSH in /etc/ssh/sshd_config: Enable root access, allow password authentication and empty password if you need.
# Also add aliases like alias uh='ls -l --group-directories-first' in /etc/profile
cd ..
sudo umount ext4
```

Remove loopback

```
sudo losetup -d /dev/loopX
```

### Write the image to an SD card

Remember to change /dev/sdX to the right value for your SD card!

```
# cd to de10 directory
sudo dd if=de10_nano_sd.img of=/dev/sdX bs=64K status=progress
sync
```

Pop the SD card into the DE10 nano, and it should boot up into Buildroot.

## HPS communication via USB

Use a serial device tool like tio to communicate with the HPS. Set the baud rate accordingly if necessary.

```
tio /dev/ttyUSB0
tio -b 57600 -d 8 -f none -s 1 -p none /dev/ttyUSB0
```

## Access the FPGA logic via HPS

On Buildroot you can utilize the `devmem` command to access the entire physical address space, including the FPGA Slaves (AXI), the FPGA Lightweight (AXI Lite), the common SDRAM and more.
Refer to the [Cyclone V HPS Technical Reference Manual](https://www.intel.com/content/www/us/en/docs/programmable/683126/21-2/hard-processor-system-technical-reference.html). Refer to `src/soc.qsys` in Quartus for FPGA memory mapped slave base addresses.

For a quick example on how to remotely access some FPGA status registers, run `util/ReadImageMetaInfo.sh`.
