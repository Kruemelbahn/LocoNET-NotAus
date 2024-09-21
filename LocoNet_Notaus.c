/*
 * LocoNet_Notaus.c
 *
 * Created: 16.08.2014 12:52:26
 *  Author: Heiko Herholz
 */ 


#include <avr/io.h>			//Bibliothek für Ein- und Ausgabe
#include <avr/interrupt.h>	//Bibliothek für Interrupts
#include "common_defs.h"    //allgemeine Sachen aus dem LocoNet-Projekt
#include "sysdef.h"		    //Einstellungen zum Prozessor und zu diesem Projekt
#include "ln_interface.h"   //Die LocoNet-Schnittstelle
#include "loconet.h"		//LocoNet-Definitionen


LN_STATUS LnSTATUS; //LocoNet-Empfangsvariable
int taste1;			//Variable zur Entprellung von Taste 1
int taste2;			//Variable zu Entprellung von Taste 2


LN_STATUS sendLocoNet1BytePacket(byte OpCode){ //Methode zum Senden von 1-Byte-Messages
	lnMsg SendPacket;
	SendPacket.data[0]=OpCode;
	return sendLocoNetPacket(&SendPacket);
}

int main(void){
	DDRB=0xff; //alle PINs Port B auf Ausgang
	DDRC=0xff; //alle PINs Port C auf Ausgang
	DDRC &=~((1<<DDC0)|(1<<DDC1)); //PINC0 und PINC1 auf Eingang
	PORTC |=(1<<PINC0)|(1<<PINC1); //Pullup auf PINCO und PINC1 einschalten
	static LnBuf LnBuffer;
	sei(); //interrupts einschalten
	//LocoNet initialisieren
	initLnBuf(&LnBuffer);
	initLocoNet(&LnBuffer);
	LnSTATUS=sendLocoNet4BytePacket(OPC_RQ_SL_DATA, 0x00, 0x00); //Anfrage nach dem Status des Systemslots.
																 //Als Antwort sendet die Zentrale ein OPC_SL_RD_DATA
    while(1)
    {
		lnMsg *LnPacket=recvLocoNetPacket(); //neues LocoNet-Paket empfangen
		if(LnPacket->sd.command==OPC_GPON){ //Ist das neue LocoNet-Paket ein Power On?
			PORTB |=(1<<PB2); //rot aus
			PORTB &=~(1<<PB1); //grün an
		}
		if(LnPacket->sd.command==OPC_GPOFF){ //Ist das neue LocoNet-Paket ein Power off?
			PORTB |=(1<<PB1); //grün aus
			PORTB &=~(1<<PB2); //rot an
		}
		if (LnPacket->sd.command==OPC_SL_RD_DATA){
			if (LnPacket->sd.trk==0x07){ //Wertet den Gleis-Status aus. Wert aus der LocoNet-PE
				PORTB |=(1<<PB2); //rot aus
				PORTB &=~(1<<PB1); //grün an
				taste1=1;
			}
		}
		if (LnPacket->sd.command==OPC_SL_RD_DATA){
			if (LnPacket->sd.trk==0x06){ //Wertet den Gleis-Status aus. Wert aus der LocoNet-PE
				PORTB |=(1<<PB1); //grün aus
				PORTB &=~(1<<PB2); //rot an
				taste2=1;
			}
		}
		if (!(PINC&(1<<PINC0))){ //Überprüft ob PINC0 low ist
			if (taste1>=1)taste1++;
			if (taste1>15000){ //wartet 15000-Schleifen-Durchläufe
				taste1=0;
				LnSTATUS = sendLocoNet1BytePacket(OPC_GPOFF); //sendet ein Power off
				taste2=1;
			}
		}
		if (!(PINC&(1<<PINC1))){ //Überprüfz ob PINC1 low ist
			if (taste2>=1)taste2++;
			if (taste2>15000){ //wartet 15000-Schleifen-Durchläufe
				taste2=0;
				LnSTATUS = sendLocoNet1BytePacket(OPC_GPON); //sendet ein Power on
				taste1=1;
			}	
		}
    }
}