Serial communication
--------------------

> "I trust that you read the 'serial port' section of the README?" Emeric

### Communication type

Dynamixel and HerkuleX devices communicates through serial communication links.

Depending on the devices:
- Dynamixel series AX, MX-T and XL-320 uses half duplex (3 pins) TTL RS-232 links
- Dynamixel series RX, EX, MX-R, uses full duplex (4 pins) RS-485 links
- HerkuleX devices uses full duplex (4 pins) TTL RS-232 links

### Adapters

This framework can be used with any combination of RS-232 ports, USB to TTL adapters, USB to RS-485 adapters, half or full duplex... But you'll need the right link for the right device.

One more important thing: you need to power your servos with **proper power supply**. Shaky power sources have been known to cause interferences on serial links, resulting in numerous packet corruptions. Be careful when using batteries and power converters!

#### Dynamixel devices

> Note: Regular "full-duplex" TTL converters will NOT work with "half-duplex TTL" Dynamixel servos (AX serie, MX-T serie, XL-320, ...).

* [USB2AX](http://www.xevelabs.com/doku.php?id=product:usb2ax:usb2ax): Unofficial but awesome device designed to manage TTL communication with your Dynamixels (AX serie, MX-T serie, XL-320, ...).
* [USB2Dynamixel](http://support.robotis.com/en/product/auxdevice/interface/usb2dxl_manual.htm): Official device that can manage all Dynamixel devices through RS232 / RS485 / TTL communications.  
* Home made TTL half-duplex device: [like this setup](http://savageelectronics.blogspot.fr/2011/01/arduino-y-dynamixel-ax-12.html) from Savage Electronics (in spanish), or [this one](http://www.memememememememe.me/the-dynamixel/) from memememe (in english).

#### HerkuleX devices

You need a "regular" RS-232 full-duplex serial port with a TTL level converter to use HerkuleX devices.

### Serial port configuration

You will need to make sure your software can access your serial port:
* If you are running Linux, you will need special permissions from the `uucp` and/or `dialout` groups in order to access serial ports. You can add your user account to these groups with this command: `# useradd -G uucp,dialout $USER` (you'll need root credentials for this operation).
* If you are running Mac OS X, depending on your adapter, you may need to install the [FTDI driver](http://www.robotis.com/xe/download_en/646927), or the [CP210x driver](http://www.silabs.com/products/mcu/pages/usbtouartbridgevcpdrivers.aspx).
* If you are running Windows, you will need to install the [FTDI driver for the USB2Dynamixel device](http://www.robotis.com/xe/download_en/646927). You may also need other drivers depending on your adapter (like the [USB2AX driver](https://raw.githubusercontent.com/Xevel/usb2ax/master/firmware/lufa_usb2ax/USB2AX.inf), the [CP210x driver](http://www.silabs.com/products/mcu/pages/usbtouartbridgevcpdrivers.aspx), or the official [FTDI driver](http://www.ftdichip.com/Drivers/D2XX.htm)).

## About latency

The thing is: serial ports are **usually** slow and handle communication errors poorly.
- Communication speeds are low (from 115.2 KBps to 1 MBps)
- Depending on your serial port / device combination, you can even go as low as 2.4 KBps (Dynamixel PRO devices) or 57.6 KBps (HerkuleX) and as high as 3 MBps (MX devices) and even way further until 10.5 Mbps for PRO devices
- Latency over the serial bus (caused by adapters and servos) will limit the number of instructions you can send each second even more than bandwidth limitations.

A few tips to minimize latency and reduce traffic on your serial bus:  
- You can use [these tips](https://projectgus.com/2011/10/notes-on-ftdi-latency-with-arduino/) to reduce latency on serial ports adapter using FTDI chips!  
- If you are using Dynamixel devices, you may want to reduce the "Return Delay Time" value to a minimum, from the default of '250' to something like '25'. Check what value works for you.  
- For both Dynamixel and HerkuleX devices, you can set the "Status Return Level" / "Ack Policy" to '1' in order to minimize the number of status packets (only if you do not need them), or even '2' to disable them ALL. *Check your servo manual for more info on this*.  

Timeout issues are common with Dynamixel, HerkuleX (and probably most) serial devices.

* Some Linux systems are handling the FTDI latency tweaks better than other.
Some servos models can have very different response time (depending the CPU / running frequency used).
* Some particular registers can (sometimes) takes WAY more time to return values than others. For instance I think I remember MX servos and max torque values taking more than 100ms to return values. So do we want to wait for these (which would makes us use insame timeout timers) or do we just ignore them?