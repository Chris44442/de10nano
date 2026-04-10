// use std::net::{TcpListener, TcpStream};
// use std::fs::OpenOptions;
// use std::os::unix::fs::OpenOptionsExt;
// use memmap2::MmapOptions;
// // use std::slice::{from_raw_parts_mut, from_raw_parts};
// use std::slice::from_raw_parts;
// // use std::ptr::{write_volatile,read_volatile};
// use std::ptr::read_volatile;
// use std::io::Write;
// // use std::time::Duration;
// // use std::time::Instant;
//
// fn main() {
//     let _ = run_server();
// }
//
// fn run_server() -> std::io::Result<()> {
//     let listener = TcpListener::bind("0.0.0.0:8081")?;
//     println!("Listening on 0.0.0.0:8081");
//
//     loop {
//         match listener.accept() {
//             Ok((stream, addr)) => {
//                 println!("Client connected: {}", addr);
//
//                 // Handle connection in same thread (simple)
//                 if let Err(e) = handle_client(stream) {
//                     eprintln!("Connection error: {}", e);
//                 }
//
//                 println!("Client disconnected");
//             }
//             Err(e) => {
//                 eprintln!("Accept failed: {}", e);
//             }
//         }
//     }
// }
//
// const DEVMEM_PATH: &str = "/dev/msgdma_test";
//
// fn handle_client(mut stream: TcpStream) -> std::io::Result<()> {
//     let f = OpenOptions::new()
//         .read(true)
//         .custom_flags(libc::O_SYNC) // strongly ordered or non-cacheable hint, may be removed later
//         .open(DEVMEM_PATH)?;
//
//     let mmap = unsafe { MmapOptions::new().len(64).map(&f)? };
//
//     let slice: &[u8] = unsafe { std::slice::from_raw_parts(mmap.as_ptr(), mmap.len()) };
//
//     // This is the "Zero-Copy-ish" part: 
//     // The kernel will take this userspace pointer and stream it to the socket.
//     stream.write_all(slice)?;
//
//     // println!("Sent {} bytes to client", slice.len());
//     Ok(())
// }


use std::fs::OpenOptions;
use std::io::{Read, Result};
use std::ptr::{read_volatile, write_volatile};
use std::thread::sleep;
use std::time::Duration;
use memmap2::MmapOptions;

const SLOT_SIZE : usize = 1024;

fn main() -> Result<()> {
    let mut f = OpenOptions::new().read(true).write(true).open("/dev/msgdma_test")?;
    let desc_mmap = unsafe { MmapOptions::new().offset(0x2000).len(0x1000).map_mut(&f)? };
    let first_desc_ptr = desc_mmap.as_ptr() as *mut u32;

    let data_mmap = unsafe { MmapOptions::new().offset(0x0000).len(0x2000).map(&f)? };

    println!("Mmap for Descriptor and Data Buffer done");
    println!("Waiting for first IRQ");

    let mut dummy = [0u8; 1];
    let mut tail = 0; // CPU's current position in the ring

    loop {
        f.read(&mut dummy)?; // Wait for IRQ

        loop {
            let current_desc_ptr = unsafe { first_desc_ptr.add(tail * 8) };
            let ctrl_ptr = unsafe { current_desc_ptr.add(7) };
            
            let ctrl = unsafe { read_volatile(ctrl_ptr) };

            if (ctrl & (1 << 30)) == 0 {
                // Actual bytes transferred is at Word 4 (0x10)
                let actual_len = unsafe { read_volatile(current_desc_ptr.add(4)) } as usize;
                
                // Get the slice for this slot
                let start = tail * SLOT_SIZE;
                let data_slice = &data_mmap[start .. start + actual_len];

                println!("Slot {}: Received {} bytes", tail, actual_len);

                // Print as 64-bit Hex Words
                for (i, chunk) in data_slice.chunks_exact(8).enumerate() {
                    let word = u64::from_be_bytes(chunk.try_into().unwrap());
                    println!("  [{:03}] 0x{:016x}", i, word);
                }

                // Reset: Set OWN_BY_HW (bit 30) back to 1
                unsafe { write_volatile(ctrl_ptr, ctrl | (1 << 30)); }

                tail = (tail + 1) % 8;
                sleep(Duration::from_millis(1000));

            } else {
                break; // Catch up complete
            }
        }
    }
}

