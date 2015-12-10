from fcc_base_driver import FCCSerialDriver
from time import sleep

class FCCRelayDriver(FCCSerialDriver):

    def get_identity(self):
        cmd = "{IDN=1}\n"
        self.write_command(cmd)

        return self.readline_json()
	
    def set_relay(self, num, onoff):
	
        cmd = "{{R{0}={1}}}\n".format(num, onoff)
        self.write_command(cmd)
	
    def get_relay(self):
        cmd = "{Status=1}\n"
        self.write_command(cmd)
     	
        return self.readline_json()
	

def FCCRelay_Test():
    drv = FCCRelayDriver()
    drv.open_serial('SNR=95238343234351A00181')
    print(drv.dev_id)

    print(drv.get_identity())

    while (True):
        drv.set_relay(1, 0)
        drv.set_relay(2, 0)
        drv.set_relay(3, 1)
        drv.set_relay(4, 1)
        print(drv.get_relay())
        sleep(0.5)
        drv.set_relay(1, 1)
        drv.set_relay(2, 1)
        drv.set_relay(3, 0)
        drv.set_relay(4, 0)
        print(drv.get_relay())
        sleep(0.5)

FCCRelay_Test()
