/****************************************************************************
    Copyright (C) 2006 Stefan Bormann

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*****************************************************************************

 Title :   LocoNet Software UART Access library
 Author:   Stefan Bormann <stefan.bormann@gmx.de>
 Date:     17-Aug-2006
 Software:  AVR-GCC
 Target:    megaAVR (need pin change interrupt, use mega88, not mega8)

 DESCRIPTION
  Basic routines for interfacing to the LocoNet via the hardware USART.

  This LocoNet stack uses the hardware USART for reception and
  a bit banging approach for transmission.
       
 USAGE
  See the C include ln_interface.h file for a description of each function.
       
*****************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "ln_interface.h"



typedef enum
{
	FALSE_E = 0,
	TRUE_E  = 1
}
Bool_t;


static volatile LnBuf *pstLnRxBuffer;      // this queue eats received LN messages

static volatile Bool_t bCdTimerActive;     // set on reception, reset on CD timer overflow

static volatile unsigned short usCdTimerStarted;  // state of hardware timer when the last byte was received

static volatile Bool_t bBusySending;              // Signal for RX interrupt to keep hands of CD timer

/*
//#define COLLISION_MONITOR
#ifdef COLLISION_MONITOR
#define COLLISION_MONITOR_PORT PORTB
#define COLLISION_MONITOR_DDR DDRB
#define COLLISION_MONITOR_BIT PB4
#endif

//#define STARTBIT_MONITOR
#ifdef STARTBIT_MONITOR
#define STARTBIT_MONITOR_PORT PORTB
#define STARTBIT_MONITOR_DDR DDRB
#define STARTBIT_MONITOR_BIT PB4
#endif
*/

void initLocoNetHardware(LnBuf *pstRxBuffer)
{
	// Set the TX line to IDLE
	LN_SW_UART_SET_TX_HIGH
//	sbi( LN_TX_DDR, LN_TX_BIT ) ;  // This maybe was a bug in the sw_uart

	pstLnRxBuffer = pstRxBuffer ;

	// enable UART receiver and receive int
	LN_HW_UART_CONTROL_REGB = _BV(RXCIE0) | _BV(RXEN0);

	// set baud rate
	//LN_HW_UART_BAUDRATE_REG = ((F_CPU)/(16666l*16l)-1);
	LN_HW_UART_BAUDRATE_REG = (int)((F_CPU)/(16.0/0.00006)-0.5);

	// set up prescaler for CD timer
	LN_TMR_CONTROL_REG = (LN_TMR_CONTROL_REG & 0xF8) | LN_TMR_PRESCALER ;

	// Turn off the Analog Comparator
	ACSR = 1<<ACD ;
}


/////////////////////////// Network idle detection ///////////////////////////
// Using pin-change-interrupt
// All other pins in the same port may not use pin-change-interrupt


static void ResetPinChange(void)
{
	cbi(PCICR, (LN_PCI_NO / 8));   // reset interrupt enable
	sbi(PCIFR, (LN_PCI_NO / 8));   // reset int flag by writing one to it

	// enable only the RX pin to generate interrupt
#if (LN_PCI_NO / 8 == 2)
	PCMSK2 = 1 << (LN_PCI_NO%8);
#elif (LN_PCI_NO / 8 == 1)
	PCMSK1 = 1 << (LN_PCI_NO%8);
#elif (LN_PCI_NO / 8 == 0)
	PCMSK0 = 1 << (LN_PCI_NO%8);
#else
#error "to be implemented"
#endif
}


static Bool_t IsPinChanged(void)
{
	if (bit_is_set(PCIFR, LN_PCI_NO / 8))
		return TRUE_E;
	else
		return FALSE_E;
}


////////////////////////////// CD backoff timer //////////////////////////////
// The idea is to us a free running 16 bit timer as basis.
// When a busy net is detected, the current counter state is captured by software
// Possible triggers:
// - UART receive interrupt
// - break generator
// - polling of network state
// The difference of the counter state and the captured start time calculates
// the count of bits since the trigger.
// Additionally the same timer is used for bit timing in the transmit function.


static inline unsigned short BitCount2Ticks(unsigned short usBitCount)
{
	return usBitCount * LN_BIT_PERIOD;  // assumption: tick rate == F_CPU, no prescaler used
}


static void StartCdTimer(void)
{
	// timestamping *now*
	usCdTimerStarted = LN_TMR_COUNT_REG;

	// want to know, when the timer went around once
	LN_TMR_OUTP_CAPT_REG = usCdTimerStarted-1;

	// not yet wraped around -> h/w timer value may be used to determine bits since trigger
	bCdTimerActive = TRUE_E;

	// Clear the current Compare interrupt status bit and enable the Compare interrupt
	sbi(LN_TMR_INT_STATUS_REG, LN_TMR_INT_STATUS_BIT);
	sbi(LN_TMR_INT_ENABLE_REG, LN_TMR_INT_ENABLE_BIT);

	// Want to see if anything happens on the net. If not, we count the time since now to declare the net free
	ResetPinChange();
}


ISR(LN_TMR_SIGNAL)  // this timer is used to find out, that the CD backoff timer wrapped around
{                   // since it was triggered by the last reception
	cbi( LN_TMR_INT_ENABLE_REG, LN_TMR_INT_ENABLE_BIT );  // disable this interrupt
	bCdTimerActive = FALSE_E;                             // reset flag -> net is declared totally free now
}


static Bool_t IsBitCountReached(unsigned char ucBitCount)
{
	if (!bCdTimerActive)
		return TRUE_E;

	unsigned short usDelta = LN_TMR_COUNT_REG - usCdTimerStarted;

	return usDelta >= BitCount2Ticks(ucBitCount);
}


static void WaitForBitCountReached(unsigned char ucBitCount)
{
	// reset status bit, disable interrupt
	sbi(LN_TMR_INT_STATUS_REG, LN_TMR_INT_STATUS_BIT);
	cbi(LN_TMR_INT_ENABLE_REG, LN_TMR_INT_ENABLE_BIT);
	
	// calculate timer compare target for given bit count since time stamp
	LN_TMR_OUTP_CAPT_REG = usCdTimerStarted + BitCount2Ticks(ucBitCount);

	// wait for capture interrupt bit
	while (!bit_is_set(LN_TMR_INT_STATUS_REG, LN_TMR_INT_STATUS_BIT)) {}
}


///////////////////////////////// Receive ////////////////////////////////////
// We are receiving data with the UART hardware interrupt driven.
// This is as trivial as it can be.

static unsigned char ucDataOverrunError;

ISR(LN_HW_UART_RX_SIGNAL)
{
	if (LN_HW_UART_CONTROL_REGA & (1<<FE0))
	{
		// should invalidate current message in buffer, but how?
		pstLnRxBuffer->Stats.RxErrors++;
	}
	if (LN_HW_UART_CONTROL_REGA & (1<<DOR0))
	{
		ucDataOverrunError++;
	}

	// must read this register wether we have an error flag or not
	unsigned char ucRxByte = LN_HW_UART_DATA_REG;

	// want to know when CD was *started*
	if (!bBusySending)  // not messing with timer, while transmitting, because
		StartCdTimer(); // same timer is used for bit timing

	// store incoming data in queue
	addByteLnBuf(pstLnRxBuffer, ucRxByte);
}


/////////////////////////////// Transmission /////////////////////////////////
// We are transmitting bit banged without interrupts for a maximum of timing
// control and with controlled interference from other interrupt sources


#ifndef LN_SW_UART_SET_TX_LOW                               // putting a 1 to the pin to switch on NPN transistor
#define LN_SW_UART_SET_TX_LOW  sbi(LN_TX_PORT, LN_TX_BIT);  // to pull down LN line to drive low level
#endif

#ifndef LN_SW_UART_SET_TX_HIGH                              // putting a 0 to the pin to switch off NPN transistor
#define LN_SW_UART_SET_TX_HIGH cbi(LN_TX_PORT, LN_TX_BIT);  // master pull up will take care of high LN level
#endif


#define LN_COLLISION_TICKS 15


static void HandleCollision(void)
{
	StartCdTimer();  // counting bits for break from here

	LN_SW_UART_SET_TX_LOW
	sei();
	WaitForBitCountReached(LN_COLLISION_TICKS);
	LN_SW_UART_SET_TX_HIGH

	StartCdTimer();  // starting timer, now used for backoff
	bBusySending = FALSE_E;
	pstLnRxBuffer->Stats.Collisions++;
}


LN_STATUS sendLocoNetPacketTry(lnMsg *TxData, unsigned char ucPrioDelay)
{
	unsigned char ucCheckSum;
	unsigned char ucTxIndex;
	unsigned char ucTxLength = getLnMsgSize(TxData);

	// First calculate the checksum as it may not have been done
	ucCheckSum = 0xFF ;

	for(ucTxIndex = 0; ucTxIndex < ucTxLength-1; ucTxIndex++)
		ucCheckSum ^= TxData->data[ ucTxIndex ];

	TxData->data[ ucTxLength-1 ] = ucCheckSum; 

	// clip maximum prio delay???????????????????????????????????????
	if (ucPrioDelay > LN_BACKOFF_MAX)
		ucPrioDelay = LN_BACKOFF_MAX;

	// Check for current traffic here to be able to return an
	// appropriate value.
	if (IsPinChanged())
	{
		StartCdTimer();  // there was traffic, start counting backoff timer
		return LN_NETWORK_BUSY;
	}

	// If the Network is not Idle as defined by the CD time, don't start the packet yet
	if (!IsBitCountReached(LN_CARRIER_TICKS))
		return LN_CD_BACKOFF;

	// if priority delay was waited now, declare net as free for this try
	if (!IsBitCountReached(ucPrioDelay))
		return LN_PRIO_BACKOFF;

	// Check for current traffic again, because on a slow processor
	// or disabled optimisation the two "IsBitCountReached" calls 
	// together can take longer than a single bit time
	if (IsPinChanged())
	{
		StartCdTimer();  // there was traffic, start counting backoff timer
		return LN_NETWORK_BUSY;
	}

	StartCdTimer();  // either we or somebody else will start a packet now

	for (ucTxIndex=0; ucTxIndex < ucTxLength; ucTxIndex++)
	{
		volatile unsigned char ucMask = 1;   // selecting one bit in the byte
		volatile unsigned char ucDataByte = TxData->data[ucTxIndex];
		volatile unsigned char ucBitLoop;
		volatile Bool_t bLastBit = FALSE_E;  // start bit is always zero

		// We need to do this with interrupts off.
		// The last time we check for free net until sending our start bit
		// must be as short as possible, not interrupted.
		cli();
		// generating bits relative to this time stamp
		usCdTimerStarted = LN_TMR_COUNT_REG;

		// start bit
		if (!bit_is_set(LN_RX_PORT, LN_RX_BIT)) //----->---must be <2us by spec.-->-+
		{	                                    //                                  |
			sei();  // reenable interrupts, UART RX shell work...                   |
			if (ucTxIndex==0)            // first bit disturbed -> don't touch      |
			    return LN_NETWORK_BUSY;  // somebody else was faster                V
			HandleCollision();           // not first bit -> collision              |
			return LN_COLLISION;         //                                         |
		}	                      //                                                |
		LN_SW_UART_SET_TX_LOW     // Seizure of network: Begin the Start Bit <------+

		// avoid that HW UART Receive ISR messes with the timer
		bBusySending = TRUE_E;

		// data bits
		for (ucBitLoop=0; ucBitLoop<8; ucBitLoop++)
		{
			Bool_t bNewBit = (ucDataByte & ucMask) ? TRUE_E : FALSE_E;

			WaitForBitCountReached(ucBitLoop+1);  // wait for previous bit to be finished

			if (bLastBit && !bit_is_set(LN_RX_PORT, LN_RX_BIT))
			{   // last bit sent was one and we see a zero on the net -> collision detected
				HandleCollision();
				return LN_COLLISION;
			}

			if (bNewBit)
				LN_SW_UART_SET_TX_HIGH
			else
				LN_SW_UART_SET_TX_LOW

			bLastBit = bNewBit;
			ucMask <<= 1;
		}

		// check eighth bit for collision
		WaitForBitCountReached(9);
		if (bLastBit && !bit_is_set(LN_RX_PORT, LN_RX_BIT))
		{	// last bit sent was one and we see a zero on the net -> collision detected
			HandleCollision();
			return LN_COLLISION;
		}
		LN_SW_UART_SET_TX_HIGH;   // send stop bit

		sei();  // not time critical from this point
		        // window for other interrupts between begin of endbit and begin of next start bit

		// check for collision in stop bit
		WaitForBitCountReached(10);  // complete stop bit
		if (!bit_is_set(LN_RX_PORT, LN_RX_BIT))
		{	// stop bit sent was zero on the net -> collision detected
			HandleCollision();
			return LN_COLLISION;
		}
	}
	StartCdTimer();
	bBusySending = FALSE_E;

	pstLnRxBuffer->Stats.TxPackets++ ;

	return LN_DONE;
}
