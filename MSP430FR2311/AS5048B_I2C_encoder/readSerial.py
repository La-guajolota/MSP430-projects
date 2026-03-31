import serial

# UART Configuration
SERIAL_PORT = '/dev/ttyACM1' # Adjust to your OS environment (/dev/ttyUSB0 on Linux)
BAUD_RATE = 115200   # Baud rate must be high enough to prevent MCU TX buffer overflow

def process_serial_data():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        
        while True:
            # 1. Wait for the sync header (0xAA)
            if ser.read(1) == b'\xAA':
                
                # 2. Read the next 3 bytes (High, Low, Terminator)
                payload = ser.read(3)
                
                if len(payload) == 3 and payload[2] == ord('\n'):
                    high_byte = payload[0]
                    low_byte = payload[1]
                    
                    # 3. Reconstruct the 16-bit integer
                    raw_angle = (high_byte << 8) | low_byte
                    
                    # 4. Mask to 14-bit (Safety measure based on AS5048B spec)
                    raw_angle &= 0x3FFF 
                    
                    # 5. Execute true mathematical interpolation
                    degrees = (raw_angle * 360.0) / 16384.0
                    
                    print(f"Raw: {raw_angle} | Angle: {degrees:.2f} deg")
                    
                    # Data is now ready to be pushed to GUI queues or CSV logs
                else:
                    # Framing error or timeout, discard and resync
                    pass
                    
    except serial.SerialException as e:
        print(f"Hardware connection error: {e}")

if __name__ == '__main__':
    process_serial_data()
