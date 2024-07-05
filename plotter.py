import matplotlib.pyplot as plt
import numpy as np

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
		if line.strip() and line.strip()[0].isdigit():
			try:
				vi_curve_data.append(list(map(lambda x: float(x.replace('V', '').replace('A', '')), line.strip().split(','))))
			except ValueError as e:
				print(f"Skipping line due to error: {line.strip()} - {e}")

    # Extracting data after setup complete
	post_setup_data = []
	for line in lines[setup_complete+2:]:
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

	fig, ax = plt.subplots()
	plt.plot(voltages, current, 'r.', label = 'Current')
	plt.plot(voltages, currentsLimit, 'g-', label = 'Current Limit')
	
	plt.xlabel('Voltage (V)')
	plt.ylabel('Current (A) / Power (W)')
	plt.grid(True)
 
	plt.plot(voltages, POWER, 'b-', label = 'Power')
	plt.legend()
	# plt.show()
	

# Function to plot post setup data
def plot_post_setup_data(data):
	if not data:
		print("No post-setup data to plot.")
		return

	modes = [item[0] for item in data]
	voltages = [item[1] for item in data]
	currents = [item[2] for item in data]
	powers = [item[3] for item in data]

	

	time = np.arange(0, len(modes)*0.1, 0.1)

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
 
	
	# plt.show()

if __name__ == "__main__":
	file_path = "log/log.txt"
	vi_curve_data, post_setup_data = parse_log_file(file_path)

	plot_vi_curve(vi_curve_data)
	plot_post_setup_data(post_setup_data)
	plt.show()
