//=== LocoNet for LocoNET-NotAus ===
#include <LocoNet.h>

//=== declaration of var's =======================================
static  lnMsg          *LnPacket;
static  LnBuf          LnTxBuffer;

/*  
 *  http://www.digitrax.com/static/apps/cms/media/documents/loconet/loconetpersonaledition.pdf
 *  http://wiki.rocrail.net/doku.php?id=loconet:ln-pe-de
 */
// default OPC's are defined in 'utility\ln_opc.h'
// A3 is already used  and defined as OPC_LOCO_F912
// A8 is already used by FRED  and defined as OPC_DEBUG
// A8 is already used by FREDI and defined as OPC_FRED_BUTTON
// AF is already used by FREDI and defined as OPC_SELFTEST resp. as OPC_FRED_ADC
// ED is already used  and defined as OPC_IMM_PACKET
// EE is already used  and defined as OPC_IMM_PACKET_2

#define  TX_PIN   7

//=== functions for receiving telegrams from SerialMonitor =======
#if defined TELEGRAM_FROM_SERIAL
static const int MAX_LEN_LNBUF = 64;
uint8_t ui8_PointToBuffer = 0;

uint8_t ui8a_receiveBuffer[MAX_LEN_LNBUF];
uint8_t ui8a_resultBuffer[MAX_LEN_LNBUF];

void ClearReceiveBuffer()
{
  ui8_PointToBuffer = 0;
  for(uint8_t i = 0; i < MAX_LEN_LNBUF; i++)
    ui8a_receiveBuffer[i] = 0;
}

uint8_t Adjust(uint8_t ui8_In)
{
  uint8_t i = ui8_In;
  if((i >= 48) && (i <= 57))
    i -= 48;
  else
    if((i >= 65) && (i <= 70))
      i -= 55;
    else
      if((i >= 97) && (i <= 102))
        i -= 87;
      else
        i = 0;
  return i;
}

uint8_t AnalyzeBuffer()
{
  uint8_t i = 0;
  uint8_t ui8_resBufSize = 0;
  while(uint8_t j = ui8a_receiveBuffer[i++])
  {
    if(j != ' ')
    {
      j = Adjust(j);
      uint8_t k = ui8a_receiveBuffer[i++];
      if(k != ' ')
        ui8a_resultBuffer[ui8_resBufSize++] = j * 16 + Adjust(k);
      else
        ui8a_resultBuffer[ui8_resBufSize++] = j;
    }
  }

  // add checksum:
  uint8_t ui8_checkSum(0xFF);
  for(i = 0; i < ui8_resBufSize; i++)
    ui8_checkSum ^= ui8a_resultBuffer[i]; 
  bitWrite(ui8_checkSum, 7, 0);     // set MSB zero
  ui8a_resultBuffer[ui8_resBufSize++] = ui8_checkSum;

  return ui8_resBufSize;
}

#endif

//=== functions ==================================================
void InitLocoNet()
{
  // First initialize the LocoNet interface, specifying the TX Pin
  LocoNet.init(TX_PIN);
  
#if defined TELEGRAM_FROM_SERIAL
  ClearReceiveBuffer();
#endif

  LocoNet.send(OPC_RQ_SL_DATA, 0x00, 0x00); // Anfrage nach dem Status des Systemslots.
                                            // Als Antwort sendet die Zentrale ein OPC_SL_RD_DATA
}

void HandleLocoNetMessages()
{
#if defined TELEGRAM_FROM_SERIAL
  LnPacket = NULL;
  if (Serial.available())
  {
    byte byte_Read = Serial.read();
    if(byte_Read == '\n')
    {
      if(AnalyzeBuffer() > 0)
        LnPacket = (lnMsg*) (&ui8a_resultBuffer);
      ClearReceiveBuffer();
    }
    else
    {
      if(ui8_PointToBuffer < MAX_LEN_LNBUF)
        ui8a_receiveBuffer[ui8_PointToBuffer++] = byte_Read;
    }
  }
#else
  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive();
#endif
  if(LnPacket)
  {
    LocoNet.processSwitchSensorMessage(LnPacket);

    // First print out the packet in HEX
    uint8_t ui8_msgLen = getLnMsgSize(LnPacket);
    uint8_t ui8_msgOPCODE = LnPacket->data[0];
    //====================================================
    if(ui8_msgLen == 14)
    {
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
      Printout('R');
#endif
      checkMsgForEmergency(ui8_msgOPCODE, LnPacket->data[7]);
    } // if(ui8_msgLen == 14)
    //====================================================
  } // if(LnPacket)
}  

#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
void Printout(char ch)
{
  if(LnPacket)
  {
    // print out the packet in HEX
    Serial.print(ch);
    Serial.print(F("X: "));
    uint8_t ui8_msgLen = getLnMsgSize(LnPacket); 
    for (uint8_t i = 0; i < ui8_msgLen; i++)
    {
      uint8_t ui8_val = LnPacket->data[i];
      // Print a leading 0 if less than 16 to make 2 HEX digits
      if(ui8_val < 16)
        Serial.print('0');
      Serial.print(ui8_val, HEX);
      Serial.print(' ');
    }
    Serial.println();
  } // if(LnPacket)
}
#endif
