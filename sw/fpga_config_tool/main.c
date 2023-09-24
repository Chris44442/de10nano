#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#define FPGA_MANAGER_REGS_ADR (0xFF706000)
#define FPGA_MANAGER_DATA_ADR (0xFFB90000)
#define CTRL_OFFSET (0x4)

const uint8_t cdratio = 0x3; // This must match the CD ratio of your MSEL setting, 0x0:CDRATIO of 1, 0x1:CDRATIO of 2, 0x2:CDRATIO of 4, 0x3:CDRATIO of 8
const char rbf_file [32] = "sdcard/fpga.rbf"; // This must match the location of your rbf file on the device
int f;
void * reg_mmap;

uint8_t fpga_state() {return *(volatile uint8_t *)(reg_mmap) & 0x7;}
void config_fpga();
void fpga_on() {*(volatile uint16_t *)(reg_mmap + CTRL_OFFSET) &= ~0x4;}
void fpga_off() {*(volatile uint16_t *)(reg_mmap + CTRL_OFFSET) |= 0x4;}
void set_axicfgen() {*(volatile uint16_t *)(reg_mmap + CTRL_OFFSET) |= 0x100;}
void reset_axicfgen() {*(volatile uint16_t *)(reg_mmap + CTRL_OFFSET) &= ~0x100;}
void set_ctrl_en() {*(volatile uint16_t *)(reg_mmap + CTRL_OFFSET) |= 0x1;}
void reset_ctrl_en() {*(volatile uint16_t *)(reg_mmap + CTRL_OFFSET) &= ~0x1;}
void set_cdratio() {
  *(volatile uint16_t *)(reg_mmap + CTRL_OFFSET) &= ~0xC0;
  *(volatile uint16_t *)(reg_mmap + CTRL_OFFSET) |= cdratio << 6;
}

int main() {
  f = open("/dev/mem", (O_RDWR|O_SYNC));
  reg_mmap = mmap(NULL, 1, (PROT_READ|PROT_WRITE), MAP_SHARED, f, FPGA_MANAGER_REGS_ADR);
  set_ctrl_en();
  fpga_off();
  while(fpga_state() != 0x1);
  set_cdratio();
  fpga_on();
  while(fpga_state() != 0x2);
  set_axicfgen(); // Activate AXI configuration data transfers.
  config_fpga();   // Load rbf config file to fpga manager data register.
  while(fpga_state() != 0x4);
  reset_axicfgen(); // Turn off AXI configuration data transfers..
  reset_ctrl_en();  // Disable control by HPS (so JTAG can load new fpga configs).
  close(f);
  return 0;
}

void config_fpga() {
  void * data_mmap = mmap(NULL, 4, (PROT_READ|PROT_WRITE), MAP_SHARED, f, FPGA_MANAGER_DATA_ADR);
  int rbf = open(rbf_file, (O_RDONLY|O_SYNC));
  if (rbf < 0) {printf("\n%s\n\n", "Error, unable to open rbf file.");exit(-1);}

  uint8_t * data_buffer = (uint8_t*)malloc(sizeof(uint8_t) * 4); // Set buffer to read rbf files and copy to fpga manager data register.
  memset(data_buffer, 0, 4); // set initial data to 0.

  bool run_while = true; // Loop to read rbf and write to fpga data address.
  while(run_while) {
    ssize_t read_result = read(rbf, data_buffer, 4); // We advancse every 4 bytes (32 bits).
    if (read_result < 4) {
      run_while = false;
    }
    uint32_t format_data = *data_buffer | (*(data_buffer+1) << 8) | (*(data_buffer+2) << 16) | (*(data_buffer+3) << 24); // Follow format expected by fpga manager.
    *(volatile uint32_t *)(data_mmap) = format_data;
    memset(data_buffer, 0, 4); // reset data to 0.
  }

  close(rbf);
}