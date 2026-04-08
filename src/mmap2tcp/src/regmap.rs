use std::fs::OpenOptions;
use std::os::unix::fs::OpenOptionsExt;
use memmap2::MmapOptions;
// use std::slice::{from_raw_parts_mut, from_raw_parts};
use std::slice::from_raw_parts;
// use std::ptr::{write_volatile,read_volatile};
use std::ptr::read_volatile;
use std::io::Write;
use std::net::TcpStream;
use std::{thread, time};
const DEVMEM_PATH: &str = "/dev/mem";

pub fn handle_client(mut stream: TcpStream) -> std::io::Result<()> {
// pub fn add_dat_to_tcp() {

    const RAM_OFFSET: usize = 0xC0003000;
    const LEN_0: usize = 12;

    let f = OpenOptions::new().read(true).custom_flags(libc::O_SYNC).write(false).create(false).open(DEVMEM_PATH).expect("Error opening file path");
    let mmap = unsafe { MmapOptions::new().offset(RAM_OFFSET as u64).len(LEN_0).map(&f).expect("Error creating mmap") };
    let slice: &[u32] = unsafe { from_raw_parts(mmap.as_ptr() as *mut u32, mmap.len() / 4) };

    // let mut stream = TcpStream::connect("192.168.0.110:8081")?;

    // for _i in 0..20 {
    loop {
        let mut numbers : Vec<u32> = vec![];
        let fifo_fill_count = unsafe{read_volatile(&slice[1])};

        // println!("fill count : {:?}", format!("0x{:08X}", fifo_fill_count));
        if fifo_fill_count > 5000 {
            println!("WARNING fifo fill count too high: {:?}", format!("0x{:08X}", fifo_fill_count));
        }

        for _j in 0..fifo_fill_count as usize {
            let fifo_content = unsafe{read_volatile(&slice[2])};
            numbers.push(fifo_content);
        }
        // debug: dbg!(&numbers);
        let buffer: Vec<u8> = numbers
            .iter()
            .flat_map(|n| n.to_be_bytes())
            .collect();

        stream.write_all(&buffer).unwrap();
        thread::sleep(time::Duration::from_millis(1));
    }
}
