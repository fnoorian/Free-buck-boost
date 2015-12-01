from drivers.fcc_base_driver_mock import FCCSerialDriver


class FCCBoostDriver(FCCSerialDriver):

    def get_identity(self):
        cmd = "{IDN=1}\n"
        self.write_command(cmd)

        return self.read_line_json()

    def set_power(self, p):
        cmd = "{{P={0}}}\n".format(round(p * 100))
        self.write_command(cmd)

    def set_volt(self, v):
        cmd = "{{V={0}}}\n".format(round(v * 100))
        self.write_command(cmd)

    def set_current(self, i):
        """Set current in amps"""
        cmd = "{{I={0}}}\n".format(round(i * 100))
        self.write_command(cmd)

    def set_duty_cycle(self, d):
        """Set duty cycle in a hundredth of percents"""
        cmd = "{{PWM={0}}}\n".format(round(d * 100))
        self.write_command(cmd)

    def set_off(self):
        cmd = "{OFF=1}\n"
        self.write_command(cmd)

    def read_status(self):
        cmd = "{STATUS=1}\n"
        self.write_command(cmd)

        return self.read_line_json()


def FCCBoostDriver_test():
    drv = FCCBoostDriver()
    drv.open_serial('SNR=95238343234351A00181')
    print(drv.dev_id)

    print(drv.get_identity())

    drv.set_mppt()
    while (True):
        print(drv.read_status())

if __name__ == '__main__':
    test = FCCBoostDriver(port_name='test')
    test.set_power(10)
    test.set_volt(100)

