extern crate memmap;
use std::ptr::{read_volatile, write_volatile};
use regmap::regmap::{H2F_LW_BASE_ADDRESS, QSYS_RAM_ADDRESS_OFFSET};

fn main() {
    const MMAPPING_BASE_ADDRESS: u64 = (H2F_LW_BASE_ADDRESS + QSYS_RAM_ADDRESS_OFFSET) as u64;
    const MMAPPING_LEN         : usize = 4096;
    const TEST_VALUE   : u32 = 0x1234_5678;
    const TEST_OFFSET  : u32 = 0x0000_0000;
    let test_address   : u32 = H2F_LW_BASE_ADDRESS + QSYS_RAM_ADDRESS_OFFSET + TEST_OFFSET/4;
    let test_word_index: usize = (TEST_OFFSET/4) as usize;

    let f = std::fs::OpenOptions::new().read(true).write(true).open("/dev/mem").unwrap();
    let mut mmap = unsafe { memmap::MmapOptions::new().offset(MMAPPING_BASE_ADDRESS).len(MMAPPING_LEN).map_mut(&f).unwrap() };
    let u32_slice = unsafe {std::slice::from_raw_parts_mut(mmap.as_mut_ptr() as *mut u32, mmap.len() / 4)};

    println!("Read at address {:#08x?}:", test_address);
    let value: u32 = unsafe {read_volatile(&u32_slice[test_word_index])};
    println!("0x{:08x?}", value);

    println!("Write 0x{:08x?} at address {:#08x?} and read back:", TEST_VALUE, test_address);
    unsafe {write_volatile(&mut u32_slice[test_word_index] as *mut u32, TEST_VALUE)};
    let value: u32 = unsafe {read_volatile(&u32_slice[test_word_index])};
    println!("0x{:08x?}", value);
}