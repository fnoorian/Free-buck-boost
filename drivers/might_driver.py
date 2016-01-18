from fcc_base_driver import FCCSerialDriver


class MightyWattDriver(FCCSerialDriver):

    def get_identity(self):
        """Get device identity"""

        self.write_json_command("IDN", 1)

        return self.readline_json()

    def set_power(self, p):
        """Set power in watts"""

        self.write_json_command("P", round(p * 1000))

    def set_current(self, i):
        """Set current in amps"""

        self.write_json_command("I", round(i * 1000))
        
    def set_volt(self, v):
        """Set voltage in volts"""

        self.write_json_command("V", round(v * 1000))
        
    def set_resistance(self, r):
        """Set load resistance in ohms"""

        self.write_json_command("R", round(r * 1000))

#    def set_off(self):
#        self.write_json_command("OFF", 1)

    def read_status_string(self):
        """Read device status as a string"""

        self.write_json_command("STATUS", 1)
        return self.readline_string()
        
    def read_status(self):
        self.write_json_command("STATUS", 1)
        return self.readline_json()

def MightyWattDriver_Test():
    drv = MightyWattDriver()
    drv.open_serial('SNR=854303535313513041E2')
    print(drv.dev_id)

    print(drv.get_identity())

    drv.set_power(4.5) # set power to 4.5 watts
    while (True):
        print(drv.read_status())

