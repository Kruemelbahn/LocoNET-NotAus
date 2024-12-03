# LocoNet-NotAus

LocoNet-NotAus is a simple circut, that sends OPC_GPOFF (0x82) for Power off or OPC_GPON (0x83) for Power on to stop or (re)start LocoNet-Traffic.<br>
Remark: if the RailSync is (also) powered off, the circuit has no powersupply and cannot work...

The current status is displayed with two addtitional LEDs.

This (my) version was adapted to compile with Arduino-IDE for Arduino-UNO.
Fuses has to be set as follows: lfuse = 0xFF; hfuse = 0xDE; efuse = 0xFD


### Requested libraries
LocoNet-NotAus requires my library listed below in addition to various Arduino standard libraries:<br> 
- [HeartBeat](https://www.github.com/Kruemelbahn/HeartBeat)

### original files and schematic
The original was developed for FREMO by H.Herholz and published here:<br>
- Hp1 Modellbahn – 3. Quartal 2010 Seite 22: "Not-Aus für Zuhaus'"<br>
- Digitale Modellbahn Heft 1/2015, Seite 60: "Notaus fürs Loconet"<br>
(sources where published on http://www.vgbahn.de/downloads/dimo/2015Heft1/loconet_notaus.zip, but are not longer available there)
