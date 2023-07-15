# OpenSensorBlue #
** OpenSensor Solution with BluetoothLE **

Ein Grossteil der Soft- und Hardware von Embedded Sensoren ist komplett uabhängig vom Endprodukt, beispielsweise das Power-Management,
Speicherzugriff, Kommunikation mit dem Benutzer, teilweise die Peripherie, ... 
Aus diesem Grund habe ich das OpenSensorBlue Framwork entwickelt, ein OpenSource Framework  für alle möglichen Arten von Sensoren, Datenloggernm, Kleinsteuerungen, ... 

Inzwischen funktioniert BluetoothLE im Browser (Android, PC und mit Einschränkunegn auch iOS) sehr zuverlässig und nicht einmal eine APP wird dazu benötigt!

Die OpenSensorBlue-Devices können von jedermann editiert, verändert, kompiliert und auf Hardware (vollständig dokumentiert) aufgespielt werden. Sämtliche benötigten
Quellcodes sind hier enthalten. Für die Kommunikation von BluetoothLE mit dem Browser ist eine Bibliothek enthalten. Für professionellen Einsatz verfügt diese Bibliothek auch
auch über professionelle Sicherheitsmechanismen und (secure) Firmware-Over-the-Air-Bootloader, aber für den Hobbygebrauch und während der Entwicklung ist dieses eher lästig, 
es geht hier wesentlich einfacher, die Software gleich direkt per JTAG aufzuspielen oder zu debuggen.

Als Basis-CPU wird die NRF52832 eingesetzt, für die es eine Unzahl sehr preisgünstiger Module gibt. 

Das erste 'Produkt' ist eine absolut profesionellee Marder-Scheuche mit Hochspannung und Ultraschall. 


# Installation
- Built with SES (V6.22a (*)) and SDK 17.1.0   (*: Later Versions of SES require changed project settings!!!)
- Uses some JesFs (OpenSource) routines
- Set Global Macro $SDK_ROOT

---
## Changelog  ##
- 15.07.2023 V1.0 

---

