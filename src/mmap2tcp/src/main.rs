use std::net::{TcpListener, TcpStream};
use std::fs::OpenOptions;
use memmap2::MmapOptions;
use std::io::Write;
use std::io::Read;
use std::ptr::{read_volatile, write_volatile};

fn main() {
    let _ = run_server();
}

fn run_server() -> std::io::Result<()> {
    let listener = TcpListener::bind("0.0.0.0:8081")?;
    println!("Listening on 0.0.0.0:8081");
    loop {
        match listener.accept() {
            Ok((stream, addr)) => {
                println!("Client connected: {}", addr);
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

const DEV_MSGDMA_PATH: &str = "/dev/msgdma_test";
const SLOT_SIZE : usize = 1024;

#[repr(C)]
struct MsgdmaStandardDesc {
    read_addr: u32,
    write_addr: u32,
    length: u32,
    next_desc: u32,
    actual_len: u32,
    status: u32,
    reserved: u32,
    control: u32,
}

use nix::poll::{poll, PollFd, PollFlags};
use std::os::unix::io::{AsRawFd, BorrowedFd};

fn handle_client(mut stream: TcpStream) -> std::io::Result<()> {
    stream.set_nodelay(true)?; 
    let mut f = OpenOptions::new().read(true).write(true).open(DEV_MSGDMA_PATH)?;
    let desc_mmap = unsafe { MmapOptions::new().offset(0x10000).len(0x1000).map_mut(&f)? };
    let first_desc_ptr = desc_mmap.as_ptr() as *mut u32;
    let data_mmap = unsafe { MmapOptions::new().offset(0x0).len(0x10000).map(&f)? };

    let mut dummy = [0u8; 1];
    let mut tail = 0; // CPU's current position in the ring
    let poll_fd = PollFd::new( unsafe { BorrowedFd::borrow_raw(f.as_raw_fd()) }, PollFlags::POLLIN);
    let desc_array: *mut MsgdmaStandardDesc = first_desc_ptr as *mut MsgdmaStandardDesc;

    loop {
        match poll(&mut [poll_fd.clone()], 5000 as u16) { // 5s timeout
            Ok(n) if n > 0 => {
                f.read(&mut dummy)?; // IRQ received, clear the wait queue signal
            }
            Ok(_) => {
                println!("Timeout: No IRQ from FPGA for 5 seconds");
                return Ok(());
            }
            Err(e) => return Err(std::io::Error::new(std::io::ErrorKind::Other, e)),
        }
        loop {
            let desc = unsafe { &mut *desc_array.add(tail) };
            let desc_control = unsafe { read_volatile(&desc.control) };
            if (desc_control & (1 << 30)) == 0 {
                let actual_len = unsafe { read_volatile(&desc.actual_len) } as usize;
                if actual_len > SLOT_SIZE {
                    println!("Error: Hardware reported invalid length {}", actual_len);
                    break;
                }
                let start = tail * SLOT_SIZE;
                let data_slice = &data_mmap[start .. start + actual_len];
                let _ = stream.write_all(&data_slice);
                unsafe { write_volatile(&mut desc.control, desc_control | (1 << 30)); }
                tail = (tail + 1) % 64;
            } else {
                break;
            }
        }
    }
}

