import matplotlib.pyplot as plt

# Function to parse the log file
def parse_log_file(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()

    # Extracting VI Curve data
    vi_curve_start = None
    setup_complete = None

    for i, line in enumerate(lines):
        if line.strip() == "VI CURVE :":
            vi_curve_start = i + 3
        if line.strip() == "SETUP COMPLETE !!":
            setup_complete = i
            break

    vi_curve_data = []
    for line in lines[vi_curve_start:setup_complete]:
        if line.strip():
            vi_curve_data.append(list(map(float, line.strip().split(','))))

    # Extracting data after setup complete
    post_setup_data = []
    for line in lines[setup_complete+2:]:
        if line.strip():
            post_setup_data.append(list(map(float, line.strip().split(','))))

    return vi_curve_data, post_setup_data

# Function to plot VI Curve data
def plot_vi_curve(data):
    voltages = [item[0] for item in data]
    currents1 = [item[1] for item in data]
    currents2 = [item[2] for item in data]

    plt.figure()
    plt.plot(voltages, currents1, label='Current 1 (A)')
    plt.plot(voltages, currents2, label='Current 2 (A)')
    plt.xlabel('Voltage (V)')
    plt.ylabel('Current (A)')
    plt.title('VI Curve')
    plt.legend()
    plt.grid(True)
    plt.show()

# Function to plot post setup data
def plot_post_setup_data(data):
    modes = [item[0] for item in data]
    voltages = [item[1] for item in data]
    currents = [item[2] for item in data]
    powers = [item[3] for item in data]

    plt.figure()
    plt.scatter(voltages, currents, c=modes, cmap='bwr', label='Voltage vs Current')
    plt.xlabel('Voltage (V)')
    plt.ylabel('Current (A)')
    plt.title('Voltage vs Current')
    plt.grid(True)
    plt.show()

    plt.figure()
    plt.scatter(powers, voltages, c=modes, cmap='bwr', label='Power vs Voltage')
    plt.xlabel('Power (W)')
    plt.ylabel('Voltage (V)')
    plt.title('Power vs Voltage')
    plt.grid(True)
    plt.show()

if __name__ == "__main__":
    file_path = "log/log.txt"
    vi_curve_data, post_setup_data = parse_log_file(file_path)
    plot_vi_curve(vi_curve_data)
    plot_post_setup_data(post_setup_data)
