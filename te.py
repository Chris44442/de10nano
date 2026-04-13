import socket
import struct

# Constants from your VHDL
HEADER_VAL = 0xaffe7788babecafe
FOOTER_VAL = 0xdeadface00c0ffee

def verify_stream(port=8082):
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", port))
    
    print(f"Monitoring Radar stream on port {port}...")
    
    expected_counter = None
    packet_idx = 0

    try:
        while True:
            data, addr = sock.recvfrom(1024)
            packet_idx += 1
            
            # 1. Check if packet is multiples of 8 bytes (64-bit words)
            if len(data) % 8 != 0:
                print(f"[Packet {packet_idx}] ERROR: Invalid length {len(data)} bytes")
                continue

            # 2. Unpack data into 64-bit unsigned integers (Little Endian)
            num_words = len(data) // 8
            words = struct.unpack(f'>{num_words}Q', data)
            
            header = words[0]
            footer = words[-1]
            payload = words[1:-1]
            
            # 3. Validate Header and Footer
            h_match = "OK" if header == HEADER_VAL else f"INVALID ({hex(header)})"
            f_match = "OK" if footer == FOOTER_VAL else f"INVALID ({hex(footer)})"
            
            # 4. Check Counter Continuity
            counter_status = "OK"
            if payload:
                first_val = payload[0]
                last_val = payload[-1]
                
                if expected_counter is not None and first_val != expected_counter:
                    counter_status = f"GAP DETECTED! Expected {expected_counter}, got {first_val}"
                    break;
                
                expected_counter = last_val + 1

            # Print Summary
            print(f"Packet: {packet_idx:5} | Words: {num_words:3} | Header: {h_match:10} | Trailer: {f_match:10} | Counter: {counter_status}")

    except KeyboardInterrupt:
        print("\nStopping verification.")
    finally:
        sock.close()

if __name__ == "__main__":
    verify_stream()
