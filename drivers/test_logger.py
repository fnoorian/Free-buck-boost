from __future__ import print_function
import time

from might_driver import MightyWattDriver
from buck_driver import FCCBuckDriver

discharger = FCCMPPTDriver()
discharger.open_serial('8543035353135160A201')
print(discharger.dev_id)

load = MightyWattDriver()
load.open_serial('854303535313513041E2')
print(load.dev_id)

# set load to something
load.set_resistance(20) # 20 ohms, at 10 volts would be 0.5 watts

# set buck to give 5 watts of power
discharger.set_power(5)

# every 250 ms, log the data
log_file = open("log.txt", "a+")
print("\nStarting:\n", file=log_file)

while True:
    try:
        time.sleep(0.250)

        load_stat = load.read_status_string()
        discharger_stat = discharger.read_status_string()

        print(load_stat, file=log_file)
        print(discharger_stat, file=log_file)

        print(load_stat)
        print(discharger_stat)

    except KeyboardInterrupt:
        print("\nKeyboard exit", file=log_file)
        break

load.close()
discharger.close()
log_file.close()

