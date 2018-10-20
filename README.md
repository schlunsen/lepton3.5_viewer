Lepton 3.5 viewer
---------------------

This is based on https://github.com/novacoast/Lepton-3-Module/tree/master/software/raspberrypi_video with modifications


This example has been tested on a Raspberry Pi 3 with Raspbian and a lepton 3.5 module[https://www.digikey.es/product-detail/en/flir/500-0771-01/500-0771-01-ND/7606616].

Enable the SPI and I2C interfaces on the Pi.

Install the 'qt4-dev-tools' package, which allows compiling of QT applications.

To build (will build any SDK dependencies as well):
qmake && make

To clean:
make sdkclean && make distclean

To start:
./viewer

If you wish to run this application without using sudo, you should add the user "pi" to the usergroup "i2c".

If you get a red square without video, click the restart button

OEM reboot was implmented with this button. Implmenting OEM reboot autimatically when the lepton is out of sync 
too long can be achieved by using the lepton_reboot() function with LeptonThread.cpp after too many out of sync
packets are recieved similarly to resync-ing using SPI port open and close.

Sometimes the lepton will achieve sync after a moment without needing to restart. It is recommended to wait a moment to see
if video starts on it's own without restart before pressing reset.

Rebooting the lepton aids in allowing it to achieve sync, this can also be done physically by removing VIN and then conecting VIN again.

----

In order for the application to run properly, a Lepton camera must be attached in a specific way to the SPI, power, and ground pins of the Raspi's GPIO interface, as well as the I2C SDA/SCL pins:

Lepton's GND pin should be connected to RasPi's ground.
Lepton's CS pin should be connected to RasPi's CE1 pin.
Lepton's MISO pin should be connected to RasPI's MISO pin.
Lepton's CLK pin should be connected to RasPI's SCLK pin.
Lepton's VIN pin should be connected to RasPI's 3v3 pin.
Lepton's SDA pin should be connected to RasPI's SDA pin.
Lepton's SCL pin should be connected to RasPI's SCL pin.
