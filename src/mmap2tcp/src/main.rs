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

fn main() -> Result<()> {
    let mut f = OpenOptions::new().read(true).write(true).open("/dev/msgdma_test")?;
    let desc_mmap = unsafe { MmapOptions::new().offset(0x1000).len(0x1000).map_mut(&f)? };
    let desc_ptr = desc_mmap.as_ptr() as *mut u32;
    let actual_buf_len_ptr = unsafe { desc_ptr.add(4) };
    let control_ptr = unsafe { desc_ptr.add(7) };

    println!("Starting Interrupt + Park Mode loop...");

    loop {
        // --- STEP 1: Wait for IRQ ---
        // This blocks until the driver calls wake_up
        let mut dummy = [0u8; 1];
        f.read(&mut dummy)?; 

        // --- STEP 2: Handle Data ---
        println!("Hello World! IRQ detected.");

        let actual_buf_len  = unsafe {read_volatile(actual_buf_len_ptr)};
        println!("actual buf len: {:?}", actual_buf_len);

        // --- STEP 3: Reset Owned by HW bit ---
        // Read current, set bit 31 (OWN_BY_HW), and write back
        unsafe {
            let mut ctrl = read_volatile(control_ptr);
            println!("ctrl before setting own_by_hw: 0x{:x?}", ctrl);
            ctrl |= (1 << 30) | (1 << 31); // Set OWN_BY_HW
            write_volatile(control_ptr, ctrl);
            let ctrl = read_volatile(control_ptr);
            println!("ctrl after: 0x{:x?}", ctrl);
        }

        println!("Bit reset. Sleeping 500ms...");
        
        // --- STEP 4: Slow it down ---
        sleep(Duration::from_millis(500));
    }
}
