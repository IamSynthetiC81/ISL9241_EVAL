import serial
from threading import Thread

import InstrumentDrivers.Keithley_DAQ6510 as MultiMeter
import InstrumentDrivers.NGL201 as Load

import time

MultimeterPresent = False
LoadPresent = False
try:
    DAQ = MultiMeter.KeithleyDAQ6510('192.168.2.11')
    MultimeterPresent = True
except:
    print("Multimeter not found")
    MultimeterPresent = False
    
try:
    NGL201 = Load.NGL201('192.168.2.12')
    LoadPresent = True
except:
    print("Load not found")
    LoadPresent = False
    
if not MultimeterPresent and not LoadPresent:
    print("No instruments found")
    StartExternalMeasurement = False
    
ExternalMeasurementLineStart = -1


# Function to log and parse serial data
def log_and_parse_serial(ser, log_file_path):
   
    current_data = []  # Temporary storage for current parsing batch
    

    with open(log_file_path, 'w') as log_file:
        StartExternalMeasurement = False
        voltage = -1
        current = -1
        battCurrent = -1
        battPower = -1
        lineCount = -1
        ExternalMeasurementLineStart = -1
        while True:
            try:
                line = ser.readline().decode('utf-8').strip()
                if not line:
                    continue
                
                lineCount += 1
                
                if StartExternalMeasurement and lineCount >= ExternalMeasurementLineStart:
                    # measure Voltage Current and Power
                    voltage = DAQ.send_command_resp('MEAS:VOLT? "voltMeasBuffer", READ')
                    current = DAQ.send_command_resp('MEAS:CURR? "currMeasBuffer", READ')
                    battCurrent = NGL201.query('MEAS:CURR?')
                    battPower = NGL201.query('MEAS:POW?')
                    
                # Print the line to the terminal
                print(line)

                if (line == 'SETUP COMPLETE !!'):
                    StartExternalMeasurement = True
                    ExternalMeasurementLineStart = lineCount + 2
                    
                    
                if (line == 'The test will initiate in 5 seconds'):
                    # restart the load
                    NGL201.send_command(":OUTP OFF")
                    time.sleep(1)
                    NGL201.send_command(":OUTP ON")

                # Log data to the file
                if (StartExternalMeasurement and lineCount >= ExternalMeasurementLineStart):
                    log_file.write(line)
                    log_file.write(f", {voltage}")
                    log_file.write(f", {current} ")
                    log_file.write(f", {battCurrent} ")
                    log_file.write(f", {battPower} \n")
                else:
                    log_file.write(line + '\n')
                
                log_file.flush()                

            except Exception as e:
                print(f"Error in serial reading: {e}")
                break


# Main function
def main():
    # Serial port configuration
    COM_PORT = 'COM6'  # Update with your port
    BAUD_RATE = 115200
    TIMEOUT = 1
    LOG_FILE_PATH = 'log\serial_log.txt'

    if MultimeterPresent:
        # # set full scale voltage and current
        DAQ.send_command(":SENS:VOLT:RANG 10")
        DAQ.send_command(":SENS:CURR:RANG 1")
        
        DAQ.send_command(":TRAC:DEL 'voltMeasBuffer'")
        DAQ.send_command(":TRAC:DEL 'currMeasBuffer'")
        
        DAQ.send_command('TRACe:MAKE "voltMeasBuffer", 10000')
        DAQ.send_command('TRACe:MAKE "currMeasBuffer", 10000')
    
        DAQ.send_command(":SYST:BEEP 1000, 0.2")
        time.sleep(0.5)
        
    if LoadPresent:
        NGL201.connect()
        NGL201.send_command("*RST;*WAI")
        NGL201.send_command("MEAS:STAT:RES")
        NGL201.send_command(":OUTP OFF")
        NGL201.set_current(1, 3)
        NGL201.set_voltage(1, 6)
        NGL201.send_command(":SYST:BEEP 1000, 1")
    
        time.sleep(1)       
    
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=TIMEOUT)
        print(f"Connected to {COM_PORT} at {BAUD_RATE} baud.")

        # Start logging and parsing in a separate thread
        log_and_parse_serial(ser, LOG_FILE_PATH)

        print("Press Ctrl+C to stop.")
        while True:
            pass  # Keep the main thread alive
    except serial.SerialException as e:
        print(f"Serial Error: {e}")
    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        if ser.is_open:
            ser.close()
        if MultimeterPresent:
            DAQ.close()
        if LoadPresent:
            NGL201.send_command(":OUTP OFF")
            NGL201.disconnect()


if __name__ == '__main__':
    main()
