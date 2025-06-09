#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "alt_types.h"
#include <stdio.h>
#include <unistd.h>

void io_switch_isr(void * context);
void io_switch_setup();
volatile int edge_val = 0;

void io_switch_setup(void){
  IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PIO_0_BASE, 0xFFFFFFFF);
  IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PIO_0_BASE, 0x0);
  void * edge_val_ptr;
  edge_val_ptr = (void *) &edge_val;
  alt_ic_isr_register(PIO_0_IRQ_INTERRUPT_CONTROLLER_ID, PIO_0_IRQ, io_switch_isr, edge_val_ptr, 0x00);
}

void io_switch_isr(void * context){
  volatile int * edge_ptr;
  edge_ptr = (volatile int *) context;
  *edge_ptr = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PIO_0_BASE);
  IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PIO_0_BASE, 0);
}

int main(void){
  printf("Entered main !\n");
  int count = 0;
  io_switch_setup();
  while(1){
  if(edge_val != 0){
    count++;
    edge_val = 0;
    printf("incrementing count : %i\n", count);
  }
  }
}
