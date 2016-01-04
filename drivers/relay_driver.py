from fcc_base_driver import FCCSerialDriver
from time import sleep

class FCCRelayDriver(FCCSerialDriver):

    def get_identity(self):
        self.write_json_command("IDN", 1)
        return self.readline_json()
	
    def set_relay(self, num, onoff):
	
        rel_number = 'R{0}'.format(num)
        self.write_json_command(rel_number, onoff)
        	
    def read_status(self):
        self.write_json_command("STATUS", 1)
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
        print(drv.read_status())
        sleep(0.5)
        drv.set_relay(1, 1)
        drv.set_relay(2, 1)
        drv.set_relay(3, 0)
        drv.set_relay(4, 0)
        print(drv.read_status())
        sleep(0.5)

FCCRelay_Test()
