from fcc_base_driver import FCCSerialDriver


class FCCBoostDriver(FCCSerialDriver):
    
    def get_identity(self):
        """Get device identity"""

        self.write_json_command("IDN", 1)
        return self.readline_json()

    def set_power(self, p):
        """Set power in watts"""

        self.write_json_command("P", round(p * 100))

    def set_volt(self, v):
        """Set voltage in volts"""

        self.write_json_command("V", round(v * 100))

    def set_current(self, i):
        """Set current in amps"""

        self.write_json_command("I", round(i * 100))

    def set_duty_cycle(self, d):
        """Set duty cycle in percents"""

        self.write_json_command("PWM", round(d * 100))

    def set_off(self):
        """Turns off all mosfets"""

        self.write_json_command("OFF", 1)

    def read_status_string(self):
        """Read device status as a string"""

        self.write_json_command("STATUS", 1)
        return self.readline_string()

    def read_status(self):
        """Read device status"""

        self.write_json_command("STATUS", 1)

        return self.readline_json()


class FCCChargerDriver(FCCBoostDriver):

    def set_charger(self):
        """Enable automatic battery charging mode"""
        self.write_json_command("BATT", 1)
        
def FCCBoostDriver_test():
    drv = FCCBoostDriver()
    drv.open_serial('SNR=95238343234351A00181')
    print(drv.dev_id)

    print(drv.get_identity())

    drv.set_power(4.5) # set power to 4.5 watts
    while (True):
        print(drv.read_status())

