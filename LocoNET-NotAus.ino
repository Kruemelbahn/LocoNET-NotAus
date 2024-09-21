/*
 * LocoNET-NotAus
 
discrete In/Outs used for functionalities:
  -  0    (used  USB)
  -  1    (used  USB)
  -  2
  -  3 Out used   LED grün (Start)
  -  4 In  used   Taster grün (Start)
  -  5 In  used   Taster rot (Stop)
  -  6 Out used   by HeartBeat
  -  7 Out used   by LocoNet [TxD]
  -  8 In  used   by LocoNet [RxD]
  -  9 Out used   LED D3 gelb, OC-Transistor (aktiv bei Notaus)
  - 10 Out used   LED rot (Stop)
  - 11
  - 12
  - 13
  - 14
  - 15
  - 16
  - 17
  - 18     (used by I²C: SDA)
  - 19     (used by I²C: SCL)

 *************************************************** 
 *  Copyright (c) 2018 Michael Zimmermann <http://www.kruemelsoft.privat.t-online.de>
 *  All rights reserved.
 *
 *  LICENSE
 *  -------
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************
 */

#include <LocoNet.h>  // requested for notifyPower

//=== global stuff =======================================
//#define DEBUG 1   // enables Outputs (debugging informations) to Serial monitor
                  // note: activating SerialMonitor in Arduino-IDE
                  //       will create a reset in software on board!
                  // please comment out also includes in system.ino

//#define DEBUG_MEM 1 // enables memory status on serial port (saves 350Bytes of code :-)

//#define TELEGRAM_FROM_SERIAL 1  // enables receiving telegrams from SerialMonitor
                                // instead from LocoNet-Port (which is inactive then)

#define ENABLE_LN             (1)
#define ENABLE_LN_E5          (1)

//#define LN_SHIELD             (1)

//========================================================

#include <HeartBeat.h>
HeartBeat oHeartbeat;

//========================================================
void setup()
{
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  // initialize serial and wait for port to open:
  Serial.begin(57600);
#endif

  InitLocoNet();
  
  InitEmergencyStop();
}

void loop()
{
  // light the Heartbeat LED
  oHeartbeat.beat();
  // generate blinken
  Blinken();

  //=== do LocoNet handling ==========
  HandleLocoNetMessages();

  //=== do EmergencyHandling handling ==========
  HandleEmergencyStop();

#if defined DEBUG
  #if defined DEBUG_MEM
    ViewFreeMemory();  // shows memory usage
    ShowTimeDiff();    // shows time for 1 cycle
  #endif
#endif
}

//=== will be called from LocoNet-Class
void notifyPower(uint8_t State)
{
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  Printout('R');
#endif
  checkMsgForEmergency(State == 0 ? OPC_GPOFF : OPC_GPON, 0);
}
