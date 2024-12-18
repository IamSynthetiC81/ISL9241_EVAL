import sys
import serial

# Function to log and parse serial data
def log_and_parse_serial(ser, log_file_path):
    current_data = []  # Temporary storage for current parsing batch
    
    with open(log_file_path, 'w') as log_file:
        lineCount = -1
        while True:
            try:
                line = ser.readline().decode('utf-8').strip()
                if not line:
                    continue
                
                lineCount += 1
                                    
                # Print the line to the terminal
                print(line)                                       

                # Log data to the file
                log_file.write(line + '\n')
                log_file.flush()                

            except Exception as e:
                print(f"Error in serial reading: {e}")
                break


# Main function
def main(COM_PORT,LOG_FILE_PATH, BAUD_RATE=115200, TIMEOUT=1):
           
    try:
        ser = serial.Serial(port=COM_PORT, baudrate=BAUD_RATE, timeout=TIMEOUT)
        print(f"Connected to {COM_PORT} at {BAUD_RATE} baud.")

        # Start logging and parsing in a separate thread
        log_and_parse_serial(ser, LOG_FILE_PATH)

        print("Press Ctrl+C to stop.")
        while True:
            pass  # Keep the main thread alive
    except serial.SerialException as e:
        if "FileNotFoundError" in str(e):
            print(f"Serial Error: {e}")
            print(f"COM Port {COM_PORT} not found.")
        elif "PermissionError" in str(e):
            print(f"Serial Error: {e}")
            print(f"COM Port {COM_PORT} is already in use.")
        elif "OSError" in str(e):
            print(f"Serial Error: {e}")
            print(f"COM Port {COM_PORT} is already in use.")
        elif "Timeout" in str(e):
            print(f"Serial Error: {e}")
            print(f"COM Port {COM_PORT} is not responding.")
        elif "UnboundLocalError" in str(e):
            print(f"Serial Error: {e}")
            print(f"COM Port {COM_PORT} is not responding.")
        else:
            print(f"Serial Error: {e}")
            return
            
        
    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        try:
            ser.close()
            print("Serial port closed.")
        except NameError:
            print("Serial port was not opened.")


if __name__ == '__main__':
    COM_PORT = sys.argv[1]      # get the COM port from the command line
    LOG_FILE_PATH = sys.argv[2] # get the log file path from the command line
    BAUD_RATE = 115200          # default baud rate
    TIMEOUT = 1                 # default timeout
    
    print(f"COM_PORT: {COM_PORT}")
    print(f"LOG_FILE_PATH: {LOG_FILE_PATH}")
    print(f"BAUD_RATE: {BAUD_RATE}")
    print(f"TIMEOUT: {TIMEOUT}")
    
    # check if BAUD_RATE and TIMEOUT are provided
    if len(sys.argv) == 4:
        BAUD_RATE = int(sys.argv[3])
        TIMEOUT = 1
    elif len(sys.argv) == 5:
        BAUD_RATE = int(sys.argv[3])
        TIMEOUT = int(sys.argv[4])
    else:
        BAUD_RATE = 115200
        TIMEOUT = 1
    
    main(COM_PORT, LOG_FILE_PATH, BAUD_RATE, TIMEOUT)
