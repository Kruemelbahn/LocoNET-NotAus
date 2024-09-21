/*
 * sysdef.h
 *
 * Created: 17.08.2014 10:20:52
 *  Author: Heiko Herholz
 */ 


#ifndef SYSDEF_H_
#define SYSDEF_H_

#define BOARD_DEFINED_IN_SYSDEF

#define LN_RX_PORT	PIND
#define LN_RX_BIT	PIND0
#define LN_TX_PORT	PORTD
#define LN_TX_DDR	DDRD
#define LN_TX_BIT	PIND1
#define LN_PCI_NO	17
#define LN_SW_UART_SET_TX_LOW {sbi(LN_TX_PORT, LN_TX_BIT);\
							   sbi(LN_TX_DDR, LN_TX_BIT);}
#define LN_SW_UART_SET_TX_HIGH {cbi(LN_TX_PORT, LN_TX_BIT);\
								cbi(LN_TX_DDR, LN_TX_BIT);}
#define LN_SB_SIGNAL			TIMER1_CAPT_vect
#define LN_SB_INT_ENABLE_REG	TIMSK1
#define LN_SB_INT_ENABLE_BIT	ICIE1
#define LN_SB_INT_STATUS_REG	TIFR1
#define LN_SB_INT_STATUS_BIT	OCF1A
#define LN_TMR_SIGNAL			TIMER1_COMPA_vect
#define LN_TMR_INT_ENABLE_REG	TIMSK1
#define LN_TMR_INT_ENABLE_BIT	OCIE1A
#define LN_TMR_INT_STATUS_REG	TIFR1
#define LN_TMR_INT_STATUS_BIT	OCF1A
#define LN_TMR_OUTP_CAPT_REG	OCR1A
#define LN_TMR_COUNT_REG		TCNT1
#define LN_TMR_CONTROL_REG			TCCR1B
#define LN_TMR_PRESCALER			1
#define LN_HW_UART_CONTROL_REGA		UCSR0A
#define LN_HW_UART_CONTROL_REGB		UCSR0B
#define LN_HW_UART_BAUDRATE_REG		UBRR0L
#define LN_HW_UART_RX_SIGNAL		USART_RX_vect
#define LN_HW_UART_DATA_REG			UDR0

#define F_CPU 8000000
#define BV(bit) _BV(bit)
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &=~_BV(bit))
#define sbi(sfr,bit)  (_SFR_BYTE(sfr) |= _BV(bit))




#endif /* SYSDEF_H_ */