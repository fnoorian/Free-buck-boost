import serial
import serial.tools.list_ports
import json

class FCCSerialDriver():

    @staticmethod
    def find_all_ports():
        return serial.tools.list_ports.comports()
    
    @staticmethod
    def find_port_for_sn(sn):
        """Returns the COM port id of a given serial number"""
        for p in serial.tools.list_ports.comports():
            if p[2].endswith(sn):
                return p[0]
        return None

    def __init__(self):
        self.port_num = None
        self.serial = None

    def open_serial(self, sn, baud = 115200):
        self.port_num = self.find_port_for_sn(sn)
        self.serial = serial.Serial(self.port_num, baud)
        self.dev_id =  self.readline_json()

    def readline_json(self):
        line = self.serial.readline()
        return json.loads(line.decode())

    def write_command(self, cmd):
        self.serial.write(cmd.encode())

    def close(self):
        if self.serial is not None:
            if self.serial.isOpen():
                self.serial.close()
                self.serial = None
                self.port_num = None


if __name__ == '__main__':
    import pprint
    pprint.pprint(FCCSerialDriver.find_all_ports())
