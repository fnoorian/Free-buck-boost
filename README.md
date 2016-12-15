Free-Buck-Boost Controller
==========================

Free-buck-boost controller is a collection of software, firmware and hardware
schematics for testing intelligent demand side management. 

These include
- a buck converter, for user controlled voltage reduction, in  applications 
  such as maximum power point tracking,
- a boost converting, for user voltage increase, in applications such as 
  battery charging, and
- a user controlled load simulator.

## Hardware

The designs use Arduino UNO <http://arduino.cc/> and custom shields.

Buck converter shield's schematics is available from
<http://www.freechargecontroller.org/>, with components available from
<http://www.freechargecontroller.org/Purchase_info>. The PCB can be ordered
from <https://breadboardkiller.com.au/designs/29-free-charge-controller-v404>.

The boost converter reuses buck's PCB, with components reordered to create a
boost circuit instead of buck.

The load is based on MightyWatt, whose schematics and other resources are
available from 
<http://kaktuscircuits.blogspot.cz/2015/03/mightywatt-resource-page.html>.

## Files

- Driver/      Python drivers for controlling different devices.
- fcc-buck/    Firmware for an Arduino based buck DC to DC converter.
- fcc-boost/   Firmware for an Arduino based boost DC to DC converter.
- mightywatt/  Firmware for an Arduino based MightyWatt shield.

## License
 * Copyright (c) 2015, Farzad Noorian <farzad.noorian@gmail.com>.
 * Copyright (c) 2015, Yousif Amjad Solaiman <https://github.com/youirepo/Free-buck-boost>.
 * Copyright (c) 2015, Jakub Polonský <http://kaktuscircuits.blogspot.cz/2015/03/mightywatt-resource-page.html>.
 * Copyright (c) 2009, Tim Nolan <http://www.timnolan.com/>.

The MightyWatt firmware, originally by Jakub Polonský, is licensed under 
Creative Commons Attribution-ShareAlike 4.0 International License.

The fcc-boost firmware and fcc-buck firmware, derived from code by Tim Nolan,
are licensed under the BSD 2-Clause License.

All Python based files are licensed under the BSD 2-Clause License.

