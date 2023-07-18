# OpenSensorBlue #
** OpenSensor Solution with BluetoothLE **

Ein Grossteil der Soft- und Hardware von Embedded Sensoren ist komplett unabhängig vom Endprodukt, beispielsweise das Power-Management,
Speicherzugriff, Kommunikation mit dem Benutzer, teilweise die Peripherie, ... 
Aus diesem Grund habe ich OpenSensorBlue entwickelt, ein OpenSource Framework für alle möglichen Arten von Sensoren, Datenloggern, Kleinsteuerungen, ... 

BluetoothLE (BLE) ist viel bequemer als Kabel und inzwischen funktioniert BLE im Browser-APP (Android, PC und mit Einschränkungen auch iOS) sehr zuverlässig und nicht einmal die Installation einer APP wird dazu benötigt!

Die Software von OpenSensorBlue-Geräten kann von jedermann editiert, verändert, kompiliert und auf Hardware (vollständig dokumentiert) aufgespielt werden. Sämtliche benötigten
Quellcodes sind hier enthalten. Inklusive einer Bibliothek für die Kommunikation per BLE. Für professionellen Einsatz verfügt diese auch
auch über Sicherheitsmechanismen und Firmware-Over-the-Air-Updates (AES-verschlüsselt), aber für den Hobbygebrauch und während der Entwicklung ist dies eher lästig, 
es geht hier wesentlich einfacher und schneller, die Software gleich direkt per JTAG aufzuspielen oder zu debuggen.

Als Basis-CPU wird die NRF52832 eingesetzt, für die es eine Unzahl sehr preisgünstiger Module gibt. 

---
# Projekte #

## 0800_Marderli ##
Das erste 'Produkt' ist eine absolut profesionellee Marder-Scheuche mit Hochspannung und Ultraschall. 
!['Marderli'](./documentation/0800_marder/img/marderli_all.jpg)

Technischer Hintergrund: Wohnend am Feldrand gibt es hier öfters Schäden durch Marder, oft auch am KFZ. Ich habe schon einiges probiert und
kann mit Sicherheit sagen, dass klassische Tipps, wie Toiletten-Steine, Hundehaar, Drahtgitter, etc. nichts nützen.
Käuflich gibt es eine Menge 'Marder-Scheuchen', speziell für KFZ, aber oft sind die technischen Angaben nicht schlüssig oder die Geräte anscheinend auch wirkungslos
oder eines hat nach Fehlfunktionen sogar die Batterie unseres Campers ruiniert durch tiefentladen. Ich habe daher den 'Marderli' für den eigenen Einsatz entwickelt.

Aus Erfahrung kann ich sagen, dass die Kombi Hochspannung und Ultraschall aber anscheinend gut funktioniert:
Das Tier bekommt einen (harmlosen!) elektrischen Schlag und lernt daraus, Plätze mit diesem charakteristischen Signal in Zukunft zu meiden.
Für die Stärke des elektrischen Schlags reichen nach gängiger Meinung ca. 0.1 - 0.25 Joule aus, und Marder können bis >30kHz hören. 
Daher liegt das Ultraschall-Signal im Bereich zwischen 18kHz und 26kHz und besteht aus 3 kurzen Tönen.
Das Signal wird alle paar Sekunden wiederholt und ist zwar unangenehm (aber noch nicht gesundheitsschädlich) laut (ca. 100 dB), aber für Menschen kaum wahrnehmbar.
Wichtig: Auch nicht hörbare, extrem laute, dauerhafte Geräusche können -bei Mensch und Tier- zu Gehörschäden führen. Dies wurde bei der Entwicklung berücksichtigt.

Besonderen Wert wurde beim 'Marderli' auf niederen Stromverbrauch gelegt, so dass das Gerät auch problemlos monatelang an Akkus betrieben werden kann und trotzdem volle Leistung bringt.
Und natürlich auf die Bedienbarkeit per BLE: So lassen sich sehr einfach jederzeit die technischen Daten (z.B. Spannung, Auslösungen, Qualität der Hochspannungs-Isolation, ...) abfragen oder ändern (z.B. BLE Name oder das Setup).

# Technische Daten: #
- konzipiert für den Einsatz 9 - 14 Volt (entweder aus Akkus oder KFZ-Bordnetz. Wichtig: ältere KFZ haben tw. Spannungsspitzen >14 Volt, siehe dazu HW-Doku!)
- Stromverbrauch besteht aus Teilkomponenten:
	- 'nur' BLE, unconnected: <20uA, 
	- 'nur' BLE, connected mit APP: <50uA
	- Hochspannungs-Erzeugung bei guter Isolation: ca. 32uA
	- Hochspannungs-Erzeugung bei schlechter Isolation: bis zu 500uA
	- Bei Kurzschluss am Hochspannungsausgang: (je nach HW-Aufbau): ca. 0.1 - 10 mA
	- Erzeugung des periodischen Ultraschall-Signals (je nach Lautstärke, siehe HW-Doku): ca. 0.1 - 1.2 mA
 
 Im Regelbetrieb (und bei mässiger Lautstärke) bleibt das Gerät also deutlich unter 0.5mA, d.h. mit Standard-Akkus der Grösse '18650' und 3500mAh 
 lassen sich, ohne Probleme und ohne die KFZ-Batterie zu benutzen mehr als 6 Monate überbrücken!
 Bei Spannungen <9V schaltet das Gerät Hochspannung und Ultraschall ab!
 
 
Link: [Zum Projekt...](./documentation/0800_marder/README.md)

## ...weitere folgen.. ##

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

