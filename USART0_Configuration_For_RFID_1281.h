/*******************************************************************************
Project 		USART0 Configuration For RFID reader on ATmega1281
Target 			ATmega1281 on STK300
Program			USART0_Configuration_For_RFID_1281.c
Author			Richard Anthony
Date			8th November 2013

// Communication setup of USART (Hyperterminal settings must match these)
//				Tx / Rx rate	Bits per second		9600
//				Data bits							   8
//				Parity								None
//				Stop bits							   1
//				Flow control						 H/W

// PORTS
//				USART on PortE
//				USART0 RxD is on Port E bit 0 (Input)	// Read data from RFID reader
//				USART0 TxD is on Port E bit 1 (Output)	// Not used (do not send anything to RFID reader)
*******************************************************************************/
void USART_SETUP_9600_BAUD_ASSUME_1MHz_CLOCK();

unsigned char VALID = 0; // 0 = not valid, 1 = valid
void USART_SETUP_9600_BAUD_ASSUME_1MHz_CLOCK()
{
	//UCSR0A – USART Control and Status Register A
	// bit 7 RXC Receive Complete (flag)
	// bit 6 TXC Transmit Complete (flag)
	// bit 5 UDRE Data Register Empty (flag)
	// bit 4 FE Frame Error (flag) - programmatically clear this when writing to UCSRA
	// bit 3 DOR Data OverRun (flag)
	// bit 2 PE Parity Error
	// bit 1 UX2 Double the USART TX speed (but also depends on value loaded into the Baud Rate Registers)
	// bit 0 MPCM Multi-Processor Communication Mode
	UCSR1A = 0b00000010; // Set U2X (Double the USART Tx speed, to reduce clocking error)

	// UCSR0B - USART Control and Status Register B
	// bit 7 RXCIE Receive Complete Interrupt Enable
	// bit 6 TXCIE Transmit Complete Interrupt Enable
	// bit 5 UDRIE Data Register Empty Interrupt Enable
	// bit 4 RXEN Receiver Enable
	// bit 3 TXEN Transmitter Enable
	// bit 2 UCSZ2 Character Size (see also UCSZ1:0 in UCSRC)
	// 0 = 5,6,7 or 8-bit data
	// 1 = 9-bit data
	// bit 1 RXB8 RX Data bit 8 (only for 9-bit data)
	// bit 0 TXB8 TX Data bit 8 (only for 9-bit data)
	UCSR1B = 0b10010000;  // RX Complete Int Enable, RX Enable, TX NOT Enable, 8-bit data

	// UCSR0C - USART Control and Status Register C
	// *** This register shares the same I/O location as UBRRH ***
	// Bits 7:6 – UMSELn1:0 USART Mode Select (00 = Asynchronous)
	// bit 5:4 UPM1:0 Parity Mode
	// 00 Disabled
	// 10 Even parity
	// 11 Odd parity
	// bit 3 USBS Stop Bit Select
	// 0 = 1 stop bit
	// 1 = 2 stop bits
	// bit 2:1 UCSZ1:0 Character Size (see also UCSZ2 in UCSRB)
	// 00 = 5-bit data (UCSZ2 = 0)
	// 01 = 6-bit data (UCSZ2 = 0)
	// 10 = 7-bit data (UCSZ2 = 0)
	// 11 = 8-bit data (UCSZ2 = 0)
	// 11 = 9-bit data (UCSZ2 = 1)
	// bit 0 UCPOL Clock POLarity
	// 0 Rising XCK edge
	// 1 Falling XCK edge
	UCSR1C = 0b00000111;	// Asynchronous, No Parity, 1 stop, 8-bit data, Falling XCK edge

	// UBRR0 - USART0 Baud Rate Register (16-bit register, comprising UBRR0H and UBRR0L)
	UBRR1H = 0; // 9600 baud, UBRR = 12, and  U2X must be set to '1' in UCSRA
	UBRR1L = 12;
}

ISR(USART1_RX_vect) // (USART0_RX_Complete_Handler) USART0 Receive-Complete Interrupt Handler
{
	RFID_STORE_BYTE_IN_BUFFER(UDR1);	// Get received data from USART buffer and store it in RFID data buffer

	if(RFID_CODE_LENGTH <= RFID_BYTE_COUNT)	// if 16 bytes have been received, call Validation subroutine
	{
		RFID_READ_VALIDATE();	// Validate card number (check length and format are as expected)
		RFID_BYTE_COUNT = 0;	// Prepare for next card to be read
	}
}
