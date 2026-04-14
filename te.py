import socket
import struct
import time

HEADER_VAL = 0xaffe7788babecafe
FOOTER_VAL = 0xdeadface00c0ffee

def verify_stream(port=8082):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", port))
    
    print(f"Monitoring Radar stream on port {port}...")
    
    expected_counter = None
    packet_idx = 0
    
    # Performance tracking variables
    total_bytes_interval = 0
    last_report_time = time.time()

    try:
        while True:
            data, addr = sock.recvfrom(2048)
            packet_idx += 1
            total_bytes_interval += len(data)
            
            # 1. Word alignment and unpacking
            if len(data) % 8 != 0:
                print(f"[Packet {packet_idx}] ERROR: Invalid length {len(data)}")
                continue

            num_words = len(data) // 8
            # Use '>' for Big Endian to match your VHDL 0xaffe...
            words = struct.unpack(f'>{num_words}Q', data)
            
            header = words[0]
            footer = words[-1]
            payload = words[1:-1]
            
            # 2. Validation
            h_match = "OK" if header == HEADER_VAL else f"FAIL({hex(header)})"
            f_match = "OK" if footer == FOOTER_VAL else f"FAIL({hex(footer)})"
            
            # 3. Counter Check
            counter_status = "OK"
            if payload:
                first_val = payload[0]
                if expected_counter is not None and first_val != expected_counter:
                    counter_status = f"GAP! Got {first_val}, expected {expected_counter}"
                expected_counter = payload[-1] + 1

            # 4. Periodic Bitrate Reporting (every 1 second)
            current_time = time.time()
            elapsed = current_time - last_report_time
            if elapsed >= 1.0:
                bitrate_mbps = (total_bytes_interval * 8) / (elapsed * 1e6)
                print(f"\n>>> [BITRATE] {bitrate_mbps:.2f} Mbps | Total Packets: {packet_idx} <<<\n")
                
                # Reset for next interval
                total_bytes_interval = 0
                last_report_time = current_time
            
            # Optional: Limit console spam by only printing errors or every 100th packet
            if h_match != "OK" or f_match != "OK" or "GAP" in counter_status:
                print(f"Pkt: {packet_idx:5} | Hdr: {h_match} | Ftr: {f_match} | {counter_status}")

    except KeyboardInterrupt:
        print("\nStopping verification.")
    finally:
        sock.close()

if __name__ == "__main__":
    verify_stream()
