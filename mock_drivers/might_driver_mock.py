from mock_drivers.fcc_base_driver_mock import FCCSerialDriver
import json


class MightyWattDriver(FCCSerialDriver):
    def get_identity(self):
        cmd = "{IDN}\n"
        self.write_command(cmd)
        mock_read_line_json = json.loads('{"device": "MightyWatt", '
                                         '"version":1,'
                                         '"FW_VERSION": "2.3.2", '
                                         '"BOARD_REVISION": "r2.3", '
                                         '"maxIdac": 10.100, '
                                         '"maxIadc": 10.120, '
                                         '"maxVdac": 31.412, '
                                         '"maxVadc": 31.548, '
                                         '"MAX_POWER": 75.000, '
                                         '"DVM_INPUT_RESISTANCE": 330.000, '
                                         '"TEMPERATURE_THRESHOLD": 110}')
        return mock_read_line_json

    def set_power(self, p):
        cmd = "{{P={0}}}\n".format(round(p * 1000))
        self.write_command(cmd)

    def set_current(self, i):
        cmd = "{{I={0}}}\n".format(round(i * 1000))
        self.write_command(cmd)
        
    def set_volt(self, v):
        cmd = "{{V={0}}}\n".format(round(v * 1000))
        self.write_command(cmd)
        
    def set_resistance(self, r):
        cmd = "{{R={0}}}\n".format(round(r * 1000))
        self.write_command(cmd)

#    def set_off(self):
#        cmd = "{OFF=1}\n"
#        self.write_command(cmd)

    def read_status(self):
        cmd = "{MR}\n"
        self.write_command(cmd)
        mock_read_line_json = json.loads('{"current": 10.150, '
                                         '"voltage": 31.641, '
                                         '"temperature": 0, '
                                         '"remoteStatus": 0, '
                                         '"loadStatus": ["CURRENT_OVERLOAD","VOLTAGE_OVERLOAD","POWER_OVERLOAD"]}')

        return mock_read_line_json


def MightyWattDriver_Test():
    drv = MightyWattDriver()
    drv.open_serial('SNR=95238343234351A00181')
    print(drv.dev_id)

    print(drv.get_identity())

    drv.set_power(1.0)
    while (True):
        print(drv.read_status())

if __name__ == '__main__':
    test = MightyWattDriver(port_name='port_3')
    test.set_power(10)
    test.set_volt(1001)
    print(test.read_status())
    print(test.get_identity())

