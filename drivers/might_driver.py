from fcc_base_driver import FCCSerialDriver


class MightyWattDriver(FCCSerialDriver):

    def get_identity(self):
        """Get device identity"""

        cmd = "{IDN}\n"
        self.write_command(cmd)

        return self.readline_json()

    def set_power(self, p):
        """Set power in watts"""

        cmd = "{{P={0}}}\n".format(round(p * 1000))
        self.write_command(cmd)

    def set_current(self, i):
        """Set current in amps"""

        cmd = "{{I={0}}}\n".format(round(i * 1000))
        self.write_command(cmd)
        
    def set_volt(self, v):
        """Set voltage in volts"""

        cmd = "{{V={0}}}\n".format(round(v * 1000))
        self.write_command(cmd)
        
    def set_resistance(self, r):
        """Set load resistance in volts"""

        cmd = "{{R={0}}}\n".format(round(r * 1000))
        self.write_command(cmd)

#    def set_off(self):
#        cmd = "{OFF=1}\n"
#        self.write_command(cmd)

    def read_status(self):
        cmd = "{MR}\n"
        self.write_command(cmd)

        return self.readline_json()

def MightyWattDriver_Test():
    drv = MightyWattDriver()
    drv.open_serial('SNR=95238343234351A00181')
    print(drv.dev_id)

    print(drv.get_identity())

    drv.set_power(4.5) # set power to 4.5 watts
    while (True):
        print(drv.read_status())

