import serial
import serial.tools.list_ports
import json


class FCCSerialDriver:

    @staticmethod
    def find_all_ports():
        return serial.tools.list_ports.comports()
    
    @staticmethod
    def find_port_for_sn(sn):
        # """Returns the COM port id of a given serial number"""
        for p in serial.tools.list_ports.comports():
            if p[2].endswith(sn):
                return p[0]
        return None

    def __init__(self, sn=None, port_name=None):

        if sn is not None:  # user serial number to open port
            self.open_serial(sn)
        elif port_name is not None:  # user port_name to open device
            self.open_port(port_name)
        else:
            self.serial = None  # the serial object
            self.dev_id = None  # the device ID string

    def open_port(self, port_name, baud=115200):
        # """Open device based on port name"""

        self.serial = serial.Serial(port_name, baud)
        self.dev_id = self.readline_json()

    def open_serial(self, sn, baud=115200):
        # """Open device based on serial number"""

        port_num = self.find_port_for_sn(sn)
        self.open_port(port_num, baud)

    def readline_json(self):
        line = self.serial.readline()
        # print("Read: ", line)
        return json.loads(line.decode())

    def write_command(self, cmd):
        # print("Writing: ", cmd)
        self.serial.write(cmd.encode())

    def close(self):
        if self.serial is not None:
            if self.serial.isOpen():
                self.serial.close()
                self.serial = None


if __name__ == '__main__':
    import pprint
    pprint.pprint(FCCSerialDriver.find_all_ports())
