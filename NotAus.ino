//=== NotAus for LocoNET-NotAus ===
#include <LocoNet.h>
#include <Bounce.h>

//=== declaration of var's =======================================
#if defined LN_SHIELD
  #define NOTAUS_LEDon      14
  #define NOTAUS_LEDoff     15
  #define NOTAUS_ONButton   16
  #define NOTAUS_OFFButton  17
#else
  #define NOTAUS_LEDon      3
  #define NOTAUS_LEDoff     10
  #define NOTAUS_ONButton   4
  #define NOTAUS_OFFButton  5
#endif

#define NOTAUS_Output       9
#define NOTAUS_SendIdle     17  // not usable with LN_SHIELD active!

// instanciate Bounce-Object for 20ms
Bounce bouncerOFF = Bounce(NOTAUS_OFFButton, 20);
Bounce bouncerON  = Bounce(NOTAUS_ONButton, 20);

uint8_t ui8_LEDStatusOFF = 1;  // 1 = red, 0 = green
uint8_t ui8_OFFbyMe = 0;
//=== functions ==================================================
// EmergencyStop is independent of any other functionality (except LocoNet)
void InitEmergencyStop()
{
    pinMode(NOTAUS_LEDon, OUTPUT);            // PD3 - Pin5   - lights green
    pinMode(NOTAUS_LEDoff, OUTPUT);           // PB2 - Pin16  - lights red
    pinMode(NOTAUS_ONButton, INPUT_PULLUP);   // PD4 - Pin6   - sends OPC_GPON
    pinMode(NOTAUS_OFFButton, INPUT_PULLUP);  // PD5 - Pin11  - sends OPC_GPOFF / OPC_IDLE
    pinMode(NOTAUS_Output, OUTPUT);
#if not defined LN_SHIELD
    pinMode(NOTAUS_SendIdle, INPUT_PULLUP);
#endif
}

void checkMsgForEmergency(uint8_t ui8_msgOPCODE, uint8_t ui8_msgTrack)
{
  if(ui8_msgOPCODE == OPC_GPON)
  {
    ui8_OFFbyMe = 0;
    ui8_LEDStatusOFF = 0;
  } // if(ui8_msgOPCODE == OPC_GPON)

  if((ui8_msgOPCODE == OPC_GPOFF) || (ui8_msgOPCODE == OPC_IDLE))
  {
    ui8_LEDStatusOFF = 1;
  } // if((ui8_msgOPCODE == OPC_GPOFF) || (ui8_msgOPCODE == OPC_IDLE))

  if(ui8_msgOPCODE == OPC_SL_RD_DATA)  // needs at least one send of e.g. OPC_RQ_SL_DATA
                                       // or e.g. OPC_MOVE_SLOTS (send by FRED)
  {
    if(ui8_msgTrack == 0x06)
    { // OFF
      ui8_LEDStatusOFF = 1;
    } // if(ui8_msgTrack == 0x06)

    if(ui8_msgTrack == 0x07)
    { // ON
      ui8_OFFbyMe = 0;
      ui8_LEDStatusOFF = 0;
    } // if(ui8_msgTrack == 0x07)
  } // if(ui8_msgOPCODE == OPC_SL_RD_DATA)
}

void HandleEmergencyStop()
{
  bouncerON.update();
  bouncerON.read();
  if(bouncerON.fallingEdge())  // Pullup, Taster wird betätigt = geschlossen
  {
    LocoNet.reportPower(1);
    ui8_OFFbyMe = 0;
    ui8_LEDStatusOFF = 0;
  }

  bouncerOFF.update();
  bouncerOFF.read();
  if(bouncerOFF.fallingEdge())  // Pullup, Taster wird betätigt = geschlossen
  {
#if not defined LN_SHIELD
    lnMsg SendPacket;
    SendPacket.data[0] = (digitalRead(NOTAUS_SendIdle) == 0 ? OPC_IDLE : OPC_GPOFF);
    LocoNet.send(&SendPacket);
#else
    LocoNet.reportPower(0);
#endif
    if(ui8_LEDStatusOFF == 0)
      ui8_OFFbyMe = 1;
    ui8_LEDStatusOFF = 1;
  }

  digitalWrite(NOTAUS_Output, ui8_LEDStatusOFF);

  // attention: leds are direct: 1=on, 0=off
  digitalWrite(NOTAUS_LEDon, !ui8_LEDStatusOFF);
  digitalWrite(NOTAUS_LEDoff, (ui8_LEDStatusOFF && (!ui8_OFFbyMe || Blinken05Hz())));
}
