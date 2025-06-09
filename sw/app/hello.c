#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "alt_types.h"
#include "stdio.h"
#include "stddef.h"
#include "sys/alt_irq.h"

static void handle_button_interrupts(void * context) {
  /* Cast context to edge_capture's type. It is important that this
  be declared volatile to avoid unwanted compiler optimization. */
  volatile int * edge_capture_ptr = (volatile int * ) context;
  /* Read the edge capture register on the button PIO. Store value.*/
  * edge_capture_ptr = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PIO_0_BASE);

// int edge_val = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PIO_0_BASE);
// printf("Edge value: %x\n", edge_val);
// *edge_capture_ptr = edge_val;
  /* Write to the edge capture register to reset it. */

  // printf("doing interrupt lul \n");
  usleep(100000);
  IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PIO_0_BASE, 0);
  /* Read the PIO to delay ISR exit. This is done to prevent a
  spurious interrupt in systems with high processor -> pio
  latency and fast interrupts. */
  IORD_ALTERA_AVALON_PIO_EDGE_CAP(PIO_0_BASE);
}

/* Declare a global variable to hold the edge capture value. */
volatile int edge_capture = 0;
/* Initialize the button_pio. */
static void init_button_pio()
{
 /* Recast the edge_capture pointer to match the
 alt_irq_register() function prototype. */
 void * edge_capture_ptr = (void * ) & edge_capture;
 /* Enable all 4 button interrupts. */
 IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PIO_0_BASE, 0xf);
 /* Reset the edge capture register. */
 IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PIO_0_BASE, 0x0);
 /* Register the ISR. */
 alt_ic_isr_register(PIO_0_IRQ_INTERRUPT_CONTROLLER_ID,
 PIO_0_IRQ,
 handle_button_interrupts,
 edge_capture_ptr, 0x0);
}


int main(void){
  printf("Entered 15 main !\n");

  init_button_pio();

  int count = 0;
  while(1){
    count++;
    usleep(80000);
    if(edge_capture > 0){
      edge_capture = 0;
      printf("EDGE CAPTURED, YAYYYY\n");
    } else {
      printf("edge not captured, in while loop %i \n", count);
    }
  }
}

