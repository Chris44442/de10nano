// use std::env;

// fn main() {
  // let args: Vec<String> = env::args().collect();
  // let query = &args[1];
  // let filename = &args[2];
  // println!("Searching for {}", query);
  // println!("In file {}", filename);
// }


use std::fs;

extern crate memmap;
use memmap::MmapOptions;

pub fn main() {
  const PERIPHERAL_BASE_ADDRESS: u64 = 0xFF20_0000;
  const SYSTEM_TIMER_OFFSET: u64 = 0x0000;

  let f = fs::OpenOptions::new().read(true).write(true).open("/dev/mem").unwrap();
  let mmap = unsafe {MmapOptions::new().offset(PERIPHERAL_BASE_ADDRESS + SYSTEM_TIMER_OFFSET).len(4096).map_mut(&f).unwrap()};

  let bytes = mmap.get(0..4).unwrap();
  println!("{:x?}", bytes[0]);
}