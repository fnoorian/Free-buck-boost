from fcc_base_driver import FCCSerialDriver

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

