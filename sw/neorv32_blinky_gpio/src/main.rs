#![no_std]
#![no_main]

use core::panic::PanicInfo;
const GPIO_BASE: usize = 0xffff_fc08; // Adjust this according to your hardware

#[no_mangle]
pub extern "C" fn _start() -> ! {
    let gpio_output_val = (GPIO_BASE) as *mut u32;
    unsafe {
        gpio_output_val.write_volatile(0x3333333);
        loop {
        }
    }
}

#[panic_handler]
fn panic (_info: &PanicInfo) -> ! {
    loop {}
}

