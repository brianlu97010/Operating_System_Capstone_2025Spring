import struct
import serial
import time
import sys

def calculate_checksum(data):
    # Sum in units of the 8 bits (the input type is a byte object) and use the lower 8 bits to be the checksum
    return sum(data) & 0xFF  

def send_kernel(kernel_path, serial_port, baud_rate = 115200):
    # Read the binary data from the kernel image and store it into kernel_data
    with open(kernel_path, "rb") as f:  # "rb" means open file mode ( read in binary ), it will return bytes object
        kernel_data = f.read()          # The bytes object `kernel_data` will store the full content of kernel8.img in binary, each element is a byte 

    # Calculate checksum
    checksum = calculate_checksum(kernel_data)
    
    print(f"Kernel size: {len(kernel_data)} bytes")
    print(f"Checksum: 0x{checksum:08X}")
    
    # Pack the data in little endian
    header = struct.pack('<III',               # data type is unsinged int
                         0x544F4F42,           # "BOOT" in hex (little edian)
                         len(kernel_data),     # kerenl size
                         checksum              # checksum
                        )              
    
    try:
        # Write to the serial port
        with serial.Serial(serial_port, baud_rate, write_timeout=1) as s:
            print("Sending header...")
            bytes_written = s.write(header)
            time.sleep(1)       # 不加傳不過去 ?
            s.flush()
            print(f"Header sent {bytes_written} bytes successfully!")
            
            time.sleep(1)
            print("Sending kernel...")
            bytes_written = s.write(kernel_data)
            s.flush()
        
            print(f"Kernel sent {bytes_written} bytes successfully!")
            
    except Exception as e:
        print(f"Error sending data: {e}")
    except serial.SerialTimeoutException:
        print("Error: write operation time out")      
        
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <kernel_file> <serial_device>")
        sys.exit(1)
    
    kernel_path = sys.argv[1]
    serial_device = sys.argv[2]
    send_kernel(kernel_path, serial_device)