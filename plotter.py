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
	post_setup_data = []
	for line in lines[setup_complete+4:]:
		if line.strip() and line.strip()[0].isdigit():
			try:
				post_setup_data.append(list(map(float, line.strip().split(','))))
			except ValueError as e:
				print(f"Skipping line due to error: {line.strip()} - {e}")

	return vi_curve_data, post_setup_data

# Function to plot VI Curve data
def plot_vi_curve(data):
	if not data:
		print("No VI Curve data to plot.")
		return

	voltages = [item[0] for item in data]
	current = [item[1] for item in data]
	currentsLimit = [item[2] for item in data]

	POWER = np.multiply(voltages,current)

	plt.subplot(2, 1, 1)
	plt.plot(voltages, current, 'r-', label = 'Current')
	
	
	plt.xlabel('Voltage (V)')
	plt.ylabel('Current (A)')

	plt.grid(True)
 
	plt.subplot(2, 1, 2)
	plt.plot(voltages, POWER, 'b-', label = 'Power')
	plt.legend()
	plt.xlabel('Voltage (V)')
	plt.ylabel('Power (W)')

	plt.grid(True)
	# plt.show()
	

def trim_file(filename, target_string):
	# Open the file and read all lines
	with open(filename, 'r') as file:
		lines = file.readlines()

	# Find the index of the line that contains the target string
	start_index = None
	for i, line in enumerate(lines):
		if target_string in line:
			start_index = i
			break

	# If the target string is not found, do nothing
	if start_index is None:
		print(f"The string '{target_string}' was not found in the file.")
		return

	# Write the lines from the target string onward back to the file
	with open(filename, 'w') as file:
		file.writelines(lines[start_index:])

# Function to plot post setup data
def plot_post_setup_data(data, samplingPeriod = 100):
	if not data:
		print("No post-setup data to plot.")
		return

	modes = [item[0] for item in data]
	voltages = [item[1] for item in data]
	currents = [item[2] for item in data]
	powers = [item[3] for item in data]

	time = np.arange(0, len(modes)*samplingPeriod*0.1, samplingPeriod*0.1)

	f1, axis = plt.subplots(3)
 
	axis[0].plot(time, voltages, 'r-', label = 'Voltage')
	axis[0].set_ylabel('Voltage (V)')
	axis[0].set_xlabel('Time (s)')	
	axis[0].grid(True)
 
	axis[1].plot(time, currents, 'g-', label = 'Current')	
	axis[1].set_ylabel('Current (A)')
	axis[1].set_xlabel('Time (s)')
	axis[1].grid(True)
 	
	axis[2].plot(time, powers, 'b-', label = 'Power')
	axis[2].set_ylabel('Power (W)')
	axis[2].set_xlabel('Time (s)')
	axis[2].grid(True)
 

def extract_data(filename):
	# Open the file and read all lines
	with open(filename, 'r') as file:
		lines = file.readlines()
		
		for line in lines:
			if "Sampling Period : " in line:
				sampling_period = float(line.strip().split(':')[-1].strip())
				break
		else:
			print("Sampling period not found in the file.")
			return None
		
	return sampling_period

if __name__ == "__main__":
	file_path = "log/log.txt"

	# Trim the file to only include data after the MPPT Charge Controller section
	trim_file(file_path, "\t~=~=~=MPPT Charge Controller=~=~=~")

	# Extract the sampling period from the file
	sampling_period = extract_data(file_path)

	vi_curve_data, post_setup_data = parse_log_file(file_path, sampling_period = sampling_period)

	plot_vi_curve(vi_curve_data)
	plot_post_setup_data(post_setup_data)
	plt.show()
