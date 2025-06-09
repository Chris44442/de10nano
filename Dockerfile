FROM debian:bookworm-slim
RUN apt-get update
WORKDIR /home

RUN apt-get install -y wget tar bzip2
RUN wget https://toolchains.bootlin.com/downloads/releases/toolchains/armv7-eabihf/tarballs/armv7-eabihf--glibc--stable-2024.02-1.tar.bz2
RUN tar -xf armv7-eabihf--glibc--stable-2024.02-1.tar.bz2
RUN rm armv7-eabihf--glibc--stable-2024.02-1.tar.bz2
ENV CROSS_COMPILE /home/armv7-eabihf--glibc--stable-2024.02-1/bin/arm-linux-

RUN apt-get install -y git
RUN git clone https://github.com/u-boot/u-boot.git
WORKDIR /home/u-boot
RUN git checkout v2024.10

RUN apt-get install -y make gcc bison flex libssl-dev bc
RUN make ARCH=arm socfpga_de10_nano_defconfig
RUN make ARCH=arm -j $(nproc)

# apt install libncurses-dev

# git clone -b v6.12 https://github.com/torvalds/linux.git
# cd linux
# make ARCH=arm socfpga_defconfig

# to start the gui:
# make ARCH=arm menuconfig


# Compile kernel - This will take several minutes
# make ARCH=arm LOCALVERSION=zImage -j $(nproc)
# Desired output file is: linux/arch/arm/boot/zImage

# Optionally save the config for future use
# make savedefconfig
# mv defconfig arch/arm/configs/socfpga_de10_defconfig

# apt install file rsync unzip cpio g++
# export FORCE_UNSAFE_CONFIGURE=1
# cd /home/
# git clone -b 2024.11.x https://github.com/buildroot/buildroot
# cd buildroot
# make nconfig

# CONFIGURE stuff in the tui:
# Target options → Target Architecture → ARM (little endian)
# Target options → Target Architecture Variant → cortex-A9
# Target options → Target ABI → EABI
# Target options → Enable NEON SIMD extension support
# Target options → Floating point strategy → NEON
# Toolchain → Toolchain type → Buildroot toolchain
# System Configuration → System hostname → System Configuration → System banner →
# System Configuration → Init System → BusyBox
# System Configuration → /dev management → Dynamic using devtmpfs only
# System Configuration → Enable root login with password
# System Configuration → Root password → root
# System Configuration →Enable Run a getty (login prompt) after boot
# Filesystem images → Enable tar the root filesystem
# Remember to also install desired compents such as nano, openssh, compilers, etc.

