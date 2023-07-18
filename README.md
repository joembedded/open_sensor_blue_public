# OpenSensorBlue #
** OpenSensor Solution with BluetoothLE **

[<b><font color="red">GERMAN VERSION</font></b>](./documentation/README_DE.md)

A large part of the software and hardware of embedded sensors is completely independent on the end product, for example the power management,
memory access, communication with the user, partly the peripherals, ... For this reason I have developed OpenSensorBlue, an open source framework for all kinds of sensors, data loggers, small controllers, ... 

BluetoothLE (BLE) is much more convenient than cables and meanwhile BLE works very reliably in the browser APP (Android, PC and with restrictions also iOS) and not even the installation of an APP is needed!

The software of OpenSensorBlue devices can be edited, modified, compiled and installed on hardware (fully documented) by anyone. All required
source codes are included here. Including a library for communication via BLE. For professional use, this also has also has security mechanisms and firmware over-the-air updates (AES encrypted), but for hobby use and during development this is rather annoying, 
It is much easier and faster to install or debug the software directly via JTAG.

The NRF52832 is used as the basic CPU, for which there are a large number of very inexpensive modules. 

---
# Projects #

## 0800_Marderli ##
The first 'product' is an absolutely professional marten/rodent protection with high voltage and ultrasonic. 

!['Marderli'](./documentation/0800_marder/img/marderli_all.jpg)

Technical background: Living at the edge of a field, there is often damage caused by martens, often also to the car. I have already tried a few things and
can say with certainty that classic tips such as toilet stones, dog hair, wire mesh, etc. are of no use.
There are a lot of 'marten/rodent protection' devices available for sale, especially for cars, but often the technical specifications are inconclusive or the devices seem to be ineffective.
or one of them has even ruined the battery of our camper by deep-discharge after malfunctioning. I have therefore developed the 'Marderli' for my own use.

From experience I can say that the combination of high voltage and ultrasonic seems to work well:
The animal gets a (harmless!) electric shock and learns from it to avoid places with this characteristic signal in the future.
According to common opinion, about 0.1 - 0.25 joules are sufficient for the strength of the electric shock, and martens can hear up to >30kHz. 
Therefore, the ultrasonic signal is in the range between 18kHz and 26kHz and consists of 3 short tones.
The signal is repeated every few seconds and is unpleasantly (but not yet harmful to health) loud (approx. 100 dB), but hardly perceptible to humans.
Important: Even inaudible, extremely loud, continuous noises can cause hearing damage - in humans and animals. This was taken into account during development.

Special emphasis was placed on low power consumption for the 'Marderli', so that the device can be operated for months on batteries without any problems and still provide full performance.
And, of course, on the operability via BLE: Thus, the technical data (e.g. voltage, triggers, quality of the high-voltage insulation, ...) can be queried or changed (e.g. BLE name or the setup) very easily at any time.

# Technical data: #
- Designed for use with 9 - 14 volts (either from batteries or vehicle electrical system. Important: older vehicles sometimes have voltage peaks >14 volts, see HW documentation).
- Power consumption consists of subcomponents:
	- BLE 'only', unconnected: <20uA, 
	- BLE 'only', connected with APP: <50uA
	- High-voltage generation with good insulation: approx. 32uA
	- High-voltage generation with poor insulation: up to 500uA
	- In case of short-circuit at the high-voltage output: (depending on the HW structure): approx. 0.1 - 10 mA
	- Generation of the periodic ultrasonic signal (depending on volume, see HW documentation): approx. 0.1 - 1.2 mA
 
 In normal operation (and at moderate volume), the device remains well below 0.5mA, i.e. with standard batteries of the size '18650' and 3500mAh 
 you can bridge more than 6 months without problems and without using the car battery!
 At voltages <9V, the unit switches off high voltage and ultrasonic!

 
Link: [Project...](./documentation/0800_marder/README.md)

## ...more soon.. ##

---
## Tools ##

Browser-APP 'BLX.JS': [BLE API Live (im Repository)](https://joembedded.github.io/ltx_ble_demo/ble_api/index.html)

BlueShell/BLX.JS Downloads: [BlueShell/BLX.JS Home](https://joembedded.de/x3/blueshell)

---
# Installation
- Built with SES (V6.22a (*)) and SDK 17.1.0   (*: Later Versions of SES require changed project settings!!!)
- Uses some JesFs (OpenSource) routines
- Set Global Macro $SDK_ROOT

---
## Changelog  ##
- 15.07.2023 V1.0 

---

