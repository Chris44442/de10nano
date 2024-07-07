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
RUN git checkout v2024.07

RUN apt-get install -y make gcc bison flex libssl-dev bc
RUN make ARCH=arm socfpga_de10_nano_defconfig
RUN make ARCH=arm -j $(nproc)

