import socket

class NGL201:
    def __init__(self, ip, port=5025, timeout=2):
        self.ip = ip
        self.port = port
        self.timeout = timeout
        self.connection = None

    def connect(self):
        self.connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connection.settimeout(self.timeout)
        self.connection.connect((self.ip, self.port))

    def disconnect(self):
        if self.connection:
            self.connection.close()
            self.connection = None

    def send_command(self, command):
        if not self.connection:
            raise Exception("Not connected to the instrument")
        self.connection.sendall((command + '\n').encode('utf-8'))

    def query(self, command):
        self.send_command(command)
        return self.connection.recv(4096).decode('utf-8').strip()

    def set_voltage(self, channel, voltage):
        self.send_command(f'INST:NSEL {channel}')
        self.send_command(f'VOLT {voltage}')

    def set_current(self, channel, current):
        self.send_command(f'INST:NSEL {channel}')
        self.send_command(f'CURR {current}')

    def measure_voltage(self, channel):
        self.send_command(f'INST:NSEL {channel}')
        return self.query('MEAS:VOLT?')

    def measure_current(self, channel):
        self.send_command(f'INST:NSEL {channel}')
        return self.query('MEAS:CURR?')

# Example usage:
# ngl = NGL201('192.168.1.100')
# ngl.connect()
# ngl.set_voltage(1, 5.0)
# print(ngl.measure_voltage(1))
# ngl.disconnect()

if __name__ == "__main__":
    ngl = NGL201('192.168.2.12')
    ngl.connect()
    data = ngl.send_command("HCOPY:FORM?")
    print(data)
    ngl.send_command("*RST;*WAI;*CLS")