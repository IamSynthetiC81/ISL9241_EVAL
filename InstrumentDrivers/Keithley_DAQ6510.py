import socket
import time

class KeithleyDAQ6510:
    def __init__(self, ip, port=5025):
        self.ip = ip
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.ip, self.port))
        self.socket.settimeout(2)


    def get_voltage(self):
        return self.send_command(":MEAS:VOLT:DC?")

    def get_current(self):
        return self.send_command(":MEAS:CURR?")
    

    def send_command(self, command):
        self.socket.sendall((command + '\n').encode('utf-8'))
        # time.sleep(0.1)
        # Read the response
        # return self.socket.recv(4096).decode('utf-8').strip()
        
    def send_command_resp(self, command):
        self.socket.sendall((command + '\n').encode('utf-8'))
        time.sleep(0.1)
        # Read the response
        return self.socket.recv(4096).decode('utf-8').strip()
        
    def get_buffer(self):
        self.send_command(":TRAC:DATA? 1, 100, \"defbuffer1\"")
        data = self.socket.recv(4096).decode().strip()
        return data

    def close(self):
        self.socket.close()

if __name__ == "__main__":
    keithley = KeithleyDQ6510("192.168.2.11")
    keithley.close()
