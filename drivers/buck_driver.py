from drivers.fcc_base_driver import FCCSerialDriver


class FCCBuckDriver(FCCSerialDriver):

    def open(self):
        self.open_serial(self.SN_FCC_Buck)

    def get_identity(self):
        cmd = "{IDN=1}\n"
        self.write_command(cmd)

        return self.readline_json()

    def set_power(self, p):
        """Set power in watts"""
        cmd = "{{P={0}}}\n".format(round(p * 100))
        self.write_command(cmd)

    def set_volt(self, v):
        """Set voltage in volts"""
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

        return self.readline_json()


class FCCMPPTDriver(FCCBuckDriver):

    def open(self):
        self.open_serial(self.SN_FCC_MPPT)

    def set_mppt(self):
        cmd = "{MPPT=1}\n"
        self.write_command(cmd)


def FCCBuck_Test():
    drv = FCCMPPTDriver()
    drv.open_serial('SNR=95238343234351A00181')
    print(drv.dev_id)

    print(drv.get_identity())

    drv.set_mppt()
    while True:
        print(drv.read_status())

