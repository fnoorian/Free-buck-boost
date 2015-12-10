from fcc_base_driver import FCCSerialDriver

class FCCRelayDriver(FCCSerialDriver):


    def get_identity(self):
        cmd = "{IDN=1}\n"
        self.write_command(cmd)

        return self.readline_json()
	
    def set_relay(self, num, onoff):
	
        cmd = "{{R{0}={1}}}\n".format(num, onoff)
        self.write_command(cmd)
	
    def get_relay(self):
        cmd = "{ReadRel=1}\n"
        self.write_command(cmd)
     	
        return self.readline_json()
	

def FCCBuck_Test():
    drv = FCCRelayDriver()
    drv.open_serial('SNR=95238343234351A00181')
    print(drv.dev_id)

    #print(drv.get_identity())

    drv.set_relay(1, 1)
    while (True):
        print(drv.get_relay())

FCCBuck_Test()
