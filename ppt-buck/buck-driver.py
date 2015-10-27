import serial
import serial.tools.list_ports
import json
import time

class FCCSerialDriver():

    @staticmethod
    def find_port_for_sn(sn):
        """Returns the COM port id of a given serial number"""
        ports = list(serial.tools.list_ports.comports())
        for p in ports:
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

class FCCBuckDriver(FCCSerialDriver):

    SN_FCC_Buck = 'SNR=75439333635351712071'
    
    def open(self):
        self.open_serial(self.SN_FCC_Buck)

    def set_power(self, p):
        cmd = "{{P={0}}}\n".format(round(p * 100))
        self.write_command(cmd)

    def set_volt(self, v):
        cmd = "{{V={0}}}\n".format(round(v * 100))
        self.write_command(cmd)

    def set_off(self):
        cmd = "{OFF=1}\n"
        self.write_command(cmd)

    def read_status(self):
        cmd = "{STATUS=1}\n"
        self.write_command(cmd)

        return self.readline_json()

class FCCMPPTDriver(FCCBuckDriver):

    SN_FCC_MPPT = 'SNR=75439333635351918140'

    def open(self):
        self.open_serial(self.SN_FCC_MPPT)

    def set_mppt(self):
        cmd = "{MPPT=1}\n"
        self.write_command(cmd)

if __name__ == '__main__':
    drv = FCCMPPTDriver()
    drv.open()
    print(drv.dev_id)

    drv.set_mppt()
    while (True):
        print(drv.read_status())

