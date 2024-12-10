import Keithley_DAQ6510 as MultiMeter
import NGL201 as Load

import time

if __name__ == "__main__":
    # Initialize the instruments
    MultiMeter = MultiMeter.KeithleyDAQ6510('192.168.2.11')
    Load = Load.NGL201('192.168.2.12')

    # Connect to the instruments
    Load.connect()

    # beep
    MultiMeter.send_command(":SYST:BEEP 1000, 0.5")

    # # set full scale voltage and current
    MultiMeter.send_command(":SENS:VOLT:RANG 10")
    MultiMeter.send_command(":SENS:CURR:RANG 1")
    
    MultiMeter.send_command('TRACe:MAKE "voltMeasBuffer", 10000')
    MultiMeter.send_command('TRACe:MAKE "currMeasBuffer", 10000')
    
    # # measure voltage
    voltage = MultiMeter.send_command_resp('MEAS:VOLT? "voltMeasBuffer", FORM, DATE, READ')
    
    print(f"Measured Voltage: {voltage} V")
    
    time.sleep(2)
    
    # measure current
    current = MultiMeter.send_command_resp('MEAS:CURR? "currMeasBuffer", FORM, DATE, READ')
    print(f"Measured Current: {current} A")
    
    # measure power from the load
    Load.send_command(":OUTP ON")
    time.sleep(1)
    battCurrent = Load.query('MEAS:CURR?')
    battPower = Load.query('MEAS:POW?')
    print(f"Battery Current: {battCurrent} A")
    print(f"Battery Power: {battPower} W")
    
    
    
    # close the instruments
    # MultiMeter.close()
    # Load.disconnect()
    
    # clear buffers
    MultiMeter.send_command(":TRAC:CLE")
    
    # delete buffers
    MultiMeter.send_command(":TRAC:DEL 'voltMeasBuffer'")
    MultiMeter.send_command(":TRAC:DEL 'currMeasBuffer'")
    
    
    
