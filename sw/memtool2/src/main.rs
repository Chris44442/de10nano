use std::fs;
extern crate memmap;
use memmap::MmapOptions;

fn main() {
  // const H2F_LW_BASE_ADDRESS: u64 = 0xFF20_0000;
  const H2F_BASE_ADDRESS: u64 = 0xc000_0000;
  const QSYS_RAM_ADDRESS_OFFSET: u64 = 0x1000;
  
  println!("FPGA RAM address offset: {:#08x?}",H2F_BASE_ADDRESS + QSYS_RAM_ADDRESS_OFFSET);

  let f = fs::OpenOptions::new().read(true).write(true).open("/dev/mem").unwrap();
  let mut mmap = unsafe {MmapOptions::new().offset(H2F_BASE_ADDRESS + QSYS_RAM_ADDRESS_OFFSET).len(4096).map_mut(&f).unwrap()};

  println!("Read back 4 bytes of old FPGA RAM content:");
  let mut bytes = mmap.get(0..4).unwrap();
  bytes.iter().for_each(|val| print!("{:02x?} ", val));
  println!("");

  println!("Write 12 34 56 into FPGA RAM and read back 4 bytes:");
  mmap[0] = 0x12;
  mmap[1] = 0x34;
  mmap[2] = 0x56;
  bytes = mmap.get(0..4).unwrap();
  bytes.iter().for_each(|val| print!("{:02x?} ", val));
  println!("");

  println!("Write affe into FPGA RAM and read back 8 bytes:");
  mmap[0] = 0xAF;
  mmap[1] = 0xFE;
  bytes = mmap.get(0..8).unwrap();
  bytes.iter().for_each(|val| print!("{:02x?} ", val));
  println!("");
}