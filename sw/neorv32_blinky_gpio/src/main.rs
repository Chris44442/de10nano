#![no_std]
#![no_main]

use core::{panic::PanicInfo};
const GPIO_BASE: usize = 0xffff_fc08; // Adjust this according to your hardware
// const PI: f64 = 3.14159265358979323846264338327950288_f64;

#[no_mangle]
pub extern "C" fn _start() -> ! {
    let gpio_output_val = (GPIO_BASE) as *mut u32;
    unsafe {gpio_output_val.write_volatile(0x0);}

    let mut abc : f32 = 1.0;
    unsafe {gpio_output_val.read_volatile();}

    if unsafe{*gpio_output_val} == 0x0 {
        abc = 0.0;
    }
    let xyz = libm::sinf(abc);
    // let xyz = abc * 2.5;

    if xyz == 0.0 {
        unsafe {gpio_output_val.write_volatile(0x3333333);}
    } else {
        unsafe {gpio_output_val.write_volatile(0x5555555);}
    }
    loop {}
}

#[panic_handler]
fn panic (_info: &PanicInfo) -> ! {
    loop {}
}

