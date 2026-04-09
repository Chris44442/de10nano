use std::net::{TcpListener, TcpStream};
use std::fs::OpenOptions;
use std::os::unix::fs::OpenOptionsExt;
use memmap2::MmapOptions;
// use std::slice::{from_raw_parts_mut, from_raw_parts};
use std::slice::from_raw_parts;
// use std::ptr::{write_volatile,read_volatile};
use std::ptr::read_volatile;
use std::io::Write;
// use std::time::Duration;
// use std::time::Instant;

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

const DEVMEM_PATH: &str = "/dev/msgdma_test";

fn handle_client(mut stream: TcpStream) -> std::io::Result<()> {
    let f = OpenOptions::new()
        .read(true)
        .custom_flags(libc::O_SYNC) // strongly ordered or non-cacheable hint, may be removed later
        .open(DEVMEM_PATH)?;

    let mmap = unsafe { MmapOptions::new().len(64).map(&f)? };
    
    let slice: &[u8] = unsafe { std::slice::from_raw_parts(mmap.as_ptr(), mmap.len()) };

    // This is the "Zero-Copy-ish" part: 
    // The kernel will take this userspace pointer and stream it to the socket.
    stream.write_all(slice)?;

    // println!("Sent {} bytes to client", slice.len());
    Ok(())
}

