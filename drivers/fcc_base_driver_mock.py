import serial
import serial.tools.list_ports
import json


class FCCSerialDriver:

    @staticmethod
    def find_all_ports():
        return serial.tools.list_ports.comports()   # return file list

    @staticmethod
    def find_port_for_sn(sn):   # return this file name
        return sn

    def __init__(self, sn=None, port_name=None):

        if sn is not None:
            self.open_serial(sn)
        elif port_name is not None:
            self.open_port(port_name)
        else:
            self.serial = None
            self.dev_id = None

    def open_port(self, port_name, baud=115200):
        import sys, os
        self.serial = open('/home/vincent/Dropbox/Workspace/Python/Free-buck-boost/drivers/mock_ports/'+port_name, 'w+')    # should be replaced by open file
        #self.dev_id = self.read_line_json()

    def open_serial(self, sn, baud=115200):
        port_name = self.find_port_for_sn(sn)
        self.open_port(port_name, baud)

    def read_line_json(self):
        line = self.serial.readline()
        # print("Read: ", line)
        return json.loads(line)

    def write_command(self, cmd):
        # print("Writing: ", cmd)
        self.serial.write(cmd)

    def close(self):
        if self.serial is not None:
            if self.serial.readable():    # change isOpen() to readable()
                self.serial.close()
                self.serial = None


if __name__ == '__main__':
    # import pprint
    # pprint.pprint(FCCSerialDriver.find_all_ports())
    test = FCCSerialDriver(port_name='test')
    test.write_command('hahha')

