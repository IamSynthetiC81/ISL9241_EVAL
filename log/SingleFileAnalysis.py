import matplotlib.pyplot as plt
import numpy as np

# Function to parse the log file
def parse_log_file(file_path, step_size = 0.004, sampling_period = 100):
    with open(file_path, 'r') as file:
        lines = file.readlines()

    # Extracting VI Curve data
    vi_curve_start = None
    setup_complete = None

    for i, line in enumerate(lines):
        if line.strip() == "VI CURVE :":
            vi_curve_start = i + 6
        if line.strip() == "SETUP COMPLETE !!":
            setup_complete = i
            break
        if line.strip() == "minimum current : ":
            minimum_current = float(line.strip().split(':')[-1].strip())
        if line.strip() == "maximum current : ":
            maximum_current = float(line.strip().split(':')[-1].strip())
        if line.strip() == "step size : ":
            step_size = float(line.strip().split(':')[-1].strip())

    vi_curve_data = []
    for line in lines[vi_curve_start:setup_complete]:
        if line.strip() and line.strip()[0].isdigit():
            try:
                vi_curve_data.append(list(map(lambda x: float(x.replace('V', '').replace('A', '')), line.strip().split(','))))
            except ValueError as e:
                print(f"Skipping line due to error: {line.strip()} - {e}")

    # Extracting data after setup complete
    # format : dV : 3.936 dI : 0.444 : 1 : 3.50 : 1 : 3.936,0.4440,1.748,0.067,0.100
    # format : dV : DeltaV dI : DeltaI : ignore : function : ignore : Voltage,AdapterCurrent,Power,BatteryChargeCurrent,currentLimit
    post_setup_data = []
    for i,line in enumerate(lines[setup_complete+2:]):
        line.strip()
        
        # skip empty lines
        if not line:
            continue
        
        if line.startswith("Sampling Period : "):
            sampling_period = (line.split(':')[-1].strip())
        
        # # split the line by ':' and ',' and keep all values
        # line = line.split(':')
        # data = line[-1].split(',')
        
        # # append data to line
        # line.pop()
        # line.extend(data)
        
        line = line.split(',')

        # remove \n from last element
        line[-1] = line[-1].strip()
        
        if len(line) < 5:
            continue
        
        post_setup_data.append(list(map(float, line)))
    return vi_curve_data, post_setup_data, sampling_period

# Function to plot VI Curve data
def plot_vi_curve(data):
    if not data:
        print("No VI Curve data to plot.")
        return

    voltages = [item[0] for item in data]
    current = [item[1] for item in data]
    currentsLimit = [item[2] for item in data]
    
    print(voltages)

    POWER = np.multiply(voltages, current)

    plt.figure()
    plt.subplot(2, 1, 1)
    plt.plot(voltages, current, 'r-', label='Current')

    plt.plot(voltages, currentsLimit, 'b-', label='Current Limit')
    plt.xlabel('Voltage (V)')
    plt.ylabel('Current (A)')
    plt.title('VI Curve')
    plt.legend()
    plt.grid()

    plt.subplot(2, 1, 2)
    plt.plot(voltages, POWER, 'g-', label='Power')
    plt.xlabel('Voltage (V)')
    plt.ylabel('Power (W)')
    plt.title('Power Curve')
    plt.legend()
    plt.grid()

    plt.tight_layout()
    # plt.show()
    
# # Function to plot Post Setup data in time domain
def plot_post_setup_data(data , stepSize = 0.004, sampling_period = 100):
    if not data:
        print("No Post Setup data to plot.")
        return

    voltages = [item[3] for item in data]
    adapter_current = [item[4] for item in data]
    power = [item[5] for item in data]
    battery_charge_current = [item[6] for item in data]
    current_limit = [item[7] for item in data]
    MeasuredVoltage = [item[8] for item in data]
    MeasuredCurrent = [item[9] for item in data]
    MeasureBatteryChargeCurrent = [item[10] for item in data]
    MeasuredPower = [item[11] for item in data]

    MeasuredPower = -1*np.array(MeasuredPower)

    time = np.arange(0, len(voltages)*sampling_period*0.001, sampling_period*0.001)
    
    plt.figure()
    plt.subplot(3, 1, 1)
    plt.plot(time, voltages, 'r-', label = 'Voltage')
    plt.plot(time, MeasuredVoltage, 'g-', label = 'Measured Voltage')
    plt.xlabel('Time (s)')
    plt.ylabel('Voltage (V)')
    plt.title('Voltage')
    plt.legend()
    plt.grid()
    
    plt.subplot(3, 1, 2)
    plt.plot(time, adapter_current, 'g-', label = 'Adapter Current')
    plt.plot(time, MeasuredCurrent, 'r-', label = 'Measured Current')
    # plt.plot(time, battery_charge_current, 'b-', label = 'Battery Charge Current')
    # plt.plot(time, current_limit, 'y-', label = 'Current Limit')
    plt.xlabel('Time (s)')
    plt.ylabel('Current (A)')
    plt.title('Current')
    plt.legend()
    plt.grid()

    plt.subplot(3, 1, 3)
    plt.plot(time, power, 'b-', label = 'Power')
    plt.plot(time, MeasuredPower, 'r-', label = 'Measured Power')
    plt.xlabel('Time (s)')
    plt.ylabel('Power (W)')
    plt.title('Power')
    plt.legend()
    plt.grid()
    
    # plt.tight_layout()
    # plt.show()
    
def troubleShoot(vi_data,post_data, stepSize = 0.004, sampling_period = 100):
    if not post_data or not vi_data:
        print("No Post Setup data to plot.")
        return
    
    dV = [item[0] for item in post_data]
    dI = [item[1] for item in post_data]
    dP = [item[2] for item in post_data]
    Voltage = [item[3] for item in post_data]
    Current = [item[4] for item in post_data]
    Power = [item[5] for item in post_data]
    time = np.arange(0, len(dV)*sampling_period*0.001, sampling_period*0.001)
    
    
    
    plt.figure()
    plt.subplot(3, 1, 1)
    plt.plot(time, dV, 'r-', label = 'dV')
    plt.xlabel('Time (s)')
    plt.ylabel('Voltage (V)')
    plt.title('Delta Voltage')
    plt.grid()
    
    plt.subplot(3, 1, 2)
    plt.plot(time, dI, 'b-', label = 'dI')
    plt.xlabel('Time (s)')
    plt.ylabel('Current (A)')
    plt.title('Delta Current')
    plt.grid()
    

    plt.subplot(3, 1, 3)
    plt.plot(time, Power, 'r-', label = 'Power')
    plt.xlabel('Time (s)')
    plt.ylabel('Power (W)')
    plt.title('Function')
    plt.legend()
    plt.grid()
    
    

default_file_path = 'log/serial_log.txt'    

# get file_path from argument
import sys
if len(sys.argv) > 1:
    file_path = sys.argv[1]
else:
    file_path = default_file_path

print(f"Analyzing file: {file_path}")

vi_curve_data, post_setup_data, samplingPeriod = parse_log_file(file_path)

plot_vi_curve(vi_curve_data)
plot_post_setup_data(post_setup_data, samplingPeriod)
troubleShoot(vi_curve_data,post_setup_data, samplingPeriod)

# plt.tight_layout()
plt.show()
