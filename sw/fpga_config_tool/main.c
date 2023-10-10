#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

#define FPGA_MANAGER_REGS_ADR (0xFF706000)
#define FPGA_MANAGER_DATA_ADR (0xFFB90000)

const char rbf_path [32] = "sdcard/fpga.rbf"; // This must match the location of your rbf file on the device
const uint32_t cdratio = 0x3; // This must match the CD ratio of your MSEL setting, 0x0:CDRATIO of 1, 0x1:CDRATIO of 2, 0x2:CDRATIO of 4, 0x3:CDRATIO of 8

int main() {
  int devmem_file = open("/dev/mem", (O_RDWR|O_SYNC));
  uint32_t *fpga_regs = (uint32_t *)mmap(NULL, 8, PROT_READ|PROT_WRITE, MAP_SHARED, devmem_file, FPGA_MANAGER_REGS_ADR); // FPGA manager registers
  uint32_t *fpga_data = (uint32_t *)mmap(NULL, 4, PROT_READ|PROT_WRITE, MAP_SHARED, devmem_file, FPGA_MANAGER_DATA_ADR); // FPGA manager data

  int rbf_file = open(rbf_path, O_RDONLY);
  off_t rbf_size = lseek(rbf_file, 0, SEEK_END);
  lseek(rbf_file, 0, SEEK_SET);
  uint32_t *rbf_data = (uint32_t *)mmap(NULL, rbf_size, PROT_READ, MAP_PRIVATE, rbf_file, 0); // rbf data

  *(volatile uint32_t *)(fpga_regs + 1) |= 0x1; //set en (HPS takes control)
  *(volatile uint32_t *)(fpga_regs + 1) |= 0x4; //set nconfigpull (FPGA off)
  while((*(volatile uint32_t *)(fpga_regs) & 0x7) != 0x1); //wait for status update
  *(volatile uint32_t *)(fpga_regs + 1) &= ~0xC0; //reset cdratio
  *(volatile uint32_t *)(fpga_regs + 1) |= cdratio << 6; //set cdratio
  *(volatile uint32_t *)(fpga_regs + 1) &= ~0x4; //reset nconfigpull (FPGA on)
  while((*(volatile uint32_t *)(fpga_regs) & 0x7) != 0x2); //wait for status update
  *(volatile uint32_t *)(fpga_regs + 1) |= 0x100; //set axicfgen

  for (int i = 0; i < rbf_size / 4; i++) {
    fpga_data[0] = rbf_data[i];
  }

  while((*(volatile uint32_t *)(fpga_regs) & 0x7) != 0x4); //wait for status update
  *(volatile uint32_t *)(fpga_regs + 1) &= ~0x100; // reset axicfgen
  *(volatile uint32_t *)(fpga_regs + 1) &= ~0x1; // reset en (HPS releases control)

  munmap(fpga_regs, 8);
  munmap(fpga_data, 4);
  munmap(rbf_data, rbf_size);
  close(rbf_file);
  close(devmem_file);
  return 0;
}