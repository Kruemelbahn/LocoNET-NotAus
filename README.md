# LocoNet-NotAus

LocoNet-NotAus is a simple circut, that sends OPC_GPOFF (0x83) or OPC_GPON (0x83) to stop or (re)start LocoNet-Traffic.<br>
The current status is display with two addtitional LEDs.

This (my) version was adapted to compile with Arduino-IDE for Arduino-UNO.

### Requested libraries
LocoNet-NotAus requires my library listed below in addition to various Arduino standard libraries:<br> 
- [HeartBeat](https://www.github.com/Kruemelbahn/HeartBeat)

### original files and schematic
The original was developed for FREMO by H.Herholz and published here:<br>
- Hp1 Modellbahn – 3. Quartal 2010 Seite 22: "Not-Aus für Zuhaus'"<br>
- Digitale Modellbahn Heft 1/2015, Seite 60: "Notaus fürs Loconet"<br>
(sources where published on http://www.vgbahn.de/downloads/dimo/2015Heft1/loconet_notaus.zip, but are not longer available there)
