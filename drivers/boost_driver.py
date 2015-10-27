from fcc_base_driver import FCCSerialDriver

class FCCBoostDriver(FCCSerialDriver):

    SN_FCC_Boost = 'SNR=95238343234351A00181'#'SNR=75439333635351719221'
    
    def open(self):
        self.open_serial(self.SN_FCC_Buck)

    def get_identity(self):
        cmd = "{IDN=1}\n"
        self.write_command(cmd)

        return self.readline_json()

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

if __name__ == '__main__':
    drv = FCCBoostDriver()
    drv.open()
    print(drv.dev_id)

    print(drv.get_identity())

    drv.set_mppt()
    while (True):
        print(drv.read_status())

