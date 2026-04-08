use std::net::{TcpListener, TcpStream};
use std::fs::OpenOptions;
use std::os::unix::fs::OpenOptionsExt;
use memmap2::MmapOptions;
// use std::slice::{from_raw_parts_mut, from_raw_parts};
use std::slice::from_raw_parts;
// use std::ptr::{write_volatile,read_volatile};
use std::ptr::read_volatile;
use std::io::Write;
use std::time::Duration;
use std::time::Instant;

fn main() {
    let _ = run_server();
}

fn run_server() -> std::io::Result<()> {
    let listener = TcpListener::bind("0.0.0.0:8081")?;
    println!("Embedded server listening on 0.0.0.0:8081");

    loop {
        match listener.accept() {
            Ok((stream, addr)) => {
                println!("Client connected: {}", addr);

                // Handle connection in same thread (simple)
                if let Err(e) = handle_client(stream) {
                    eprintln!("Connection error: {}", e);
                }

                println!("Client disconnected");
            }
            Err(e) => {
                eprintln!("Accept failed: {}", e);
            }
        }
    }
}

const DEVMEM_PATH: &str = "/dev/mem";

fn handle_client(mut stream: TcpStream) -> std::io::Result<()> {

    const RAM_OFFSET: usize = 0xC0003000;
    const LEN_0: usize = 12;
    const MAX_CHUNK_SIZE : usize = 4096;
    const MAX_WAIT_TIME_IN_US : u64 = 200;
    const FIFO_ALMOST_FULL_COUNT : u32 = 5000;

    let f = OpenOptions::new().read(true).custom_flags(libc::O_SYNC).write(false).create(false).open(DEVMEM_PATH)?;
    let mmap = unsafe { MmapOptions::new().offset(RAM_OFFSET as u64).len(LEN_0).map(&f)? };
    let slice: &[u32] = unsafe { from_raw_parts(mmap.as_ptr() as *mut u32, mmap.len() / 4) };

    let mut buffer: Vec<u8> = Vec::with_capacity(8192);
    let mut last_flush = Instant::now();

    loop {
        let fifo_fill_count = unsafe { read_volatile(&slice[1]) };
        if fifo_fill_count > FIFO_ALMOST_FULL_COUNT {
            println!("WARNING fifo fill count too high: 0x{:08X}", fifo_fill_count);
        }

        println!("test_debug_0");
        for _ in 0..fifo_fill_count as usize {
            let value = unsafe { read_volatile(&slice[2]) };
            buffer.extend_from_slice(&value.to_be_bytes());
        }

        println!("test_debug_1");
        let should_flush_size = buffer.len() >= MAX_CHUNK_SIZE;
        let should_flush_time = last_flush.elapsed() >= Duration::from_micros(MAX_WAIT_TIME_IN_US);
        println!("test_debug_2");

        if should_flush_size || should_flush_time {
            if !buffer.is_empty() {
                stream.write_all(&buffer)?;

                buffer.clear();
                last_flush = Instant::now();
            }
        }
    }
}

