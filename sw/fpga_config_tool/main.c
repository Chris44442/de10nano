#include <stdio.h>    // printf
#include <stdint.h>   // uint8_t
#include <stdlib.h>   // malloc
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>    // open
#include <sys/mman.h> // mmap

// Useful macros
#define BIT(x,n) (((x) >> (n)) & 1)
#define INSERT_BITS(original, mask, value, num) (original & (~mask)) | (value << num)

uint8_t fpga_state();
void set_cdratio();
void config_fpga();

#define FPGA_MANAGER_ADD      (0xff706000) // FPGA MANAGER MAIN REGISTER ADDRESS
#define FPGA_MANAGER_DATA_ADD (0xffb90000) // FPGA MANAGER DATA REGISTER ADDRESS
#define CTRL_OFFSET      (0x004)

int fd; // file descriptor for memory access
void * virtualbase; // puntero genérico con map de userspace a hw
char rbf_file [32] = "sdcard/fpga.rbf";

int main(int argc, const char * argv[]) {
  const size_t largobytes = 1;

  fd = open("/dev/mem", (O_RDWR|O_SYNC));
  virtualbase = mmap(NULL, largobytes, (PROT_READ|PROT_WRITE), MAP_SHARED, fd, FPGA_MANAGER_ADD);

  *(volatile uint16_t *)(virtualbase + CTRL_OFFSET) |= 0x1;  // Activate control by HPS.
  *(volatile uint16_t *)(virtualbase + CTRL_OFFSET) |= 0x4;      // Reset State for fpga.
  while(fpga_state() != 0x1);

  set_cdratio();   // Set corresponding cdratio (check fpga manager docs).
  *(volatile uint16_t *)(virtualbase + CTRL_OFFSET) &= ~0x4;       // Should be on config state.
  while(fpga_state() != 0x2);

  *(volatile uint16_t *)(virtualbase + CTRL_OFFSET) |= 0x100; // Activate AXI configuration data transfers.
  config_fpga();   // Load rbf config file to fpga manager data register.
  while(fpga_state() != 0x4);

  *(volatile uint16_t *)(virtualbase + CTRL_OFFSET) &= ~0x100; // Turn off AXI configuration data transfers..
  *(volatile uint16_t *)(virtualbase + CTRL_OFFSET) &= ~0x1;  // Disable control by HPS (so JTAG can load new fpga configs).

  close(fd);
  return 0;
}

uint8_t fpga_state() {
  return *(volatile uint8_t *)(virtualbase) & 0x7;
}

void set_cdratio()
// This should match your MSEL Pin configuration.
// This is a config for MSEL[4..0] = 01010
{
  uint16_t control_reg  = *(volatile uint16_t *) (virtualbase + CTRL_OFFSET);
  uint16_t cdratio_mask = (0b11 << 6);
  uint8_t  cdratio      = 0x3;

  printf("Value of temp: %x\n", control_reg);

  control_reg = INSERT_BITS(control_reg, cdratio_mask, cdratio, 6);
  printf("Value of temp: %x\n", control_reg);
  *(volatile uint16_t *) (virtualbase + CTRL_OFFSET) = control_reg;

  printf("%s 0x%x.\n", "Setting cdratio with", cdratio);
}

void config_fpga() {
  void * fpga_data_mmap = mmap(NULL, 4, (PROT_READ|PROT_WRITE), MAP_SHARED, fd, FPGA_MANAGER_DATA_ADD);

  int rbf = open(rbf_file, (O_RDONLY|O_SYNC));
  if (rbf < 0) {printf("\n%s\n\n", "Error opening file. Check for an appropiate fpga_config_file.rbf file.");exit(-1);}

  // Set buffer to read rbf files and copy to fpga manager data register.
  uint8_t * data_buffer = (uint8_t*)malloc(sizeof(uint8_t) * 4);
  memset(data_buffer, 0, 4); // set initial data to 0.

  // Loop to read rbf and write to fpga data address.
  // We advancse every 4 bytes (32 bits).

  bool run_while = true;
  printf("%s\n", "Loading rbf file.");

  while(run_while) {
    ssize_t read_result = read(rbf, data_buffer, 4);
    if (read_result < 4) {
      printf("%s\n", "EOF reached.");
      run_while = false;
    }

    // Follow format expected by fpga manager.
    uint32_t format_data = *(data_buffer)          << 0;
    format_data = format_data | *(data_buffer + 1) << 8;
    format_data = format_data | *(data_buffer + 2) << 16;
    format_data = format_data | *(data_buffer + 3) << 24;

    *(volatile uint32_t *)(fpga_data_mmap) = format_data;
    memset(data_buffer, 0, 4); // reset data to 0.
  }

  close(rbf);
}