from fcc_base_driver import FCCSerialDriver

class MightyWattDriver(FCCSerialDriver):

    SN_MightyWatt = 'SNR=95238343234351A00181'#'SNR=8533434373835120D1C2'
    
    def open(self):
        self.open_serial(self.SN_MightyWatt)
        print("Device Opened");

    def get_identity(self):
        cmd = "{IDN}\n"
        self.write_command(cmd)

        return self.readline_json()

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

        return self.readline_json()

if __name__ == '__main__':
    drv = MightyWattDriver()
    drv.open()
    print(drv.dev_id)

    print(drv.get_identity())

    drv.set_power(1.0)
    while (True):
        print(drv.read_status())
