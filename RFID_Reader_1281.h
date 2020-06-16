//********************************************************************

// Main Function	Use of RFID reader ID-12
//					Configures RFID reader for ASCII output data format
//					Uses the TTL data output (D0 - this is 'inverted' (Normally High) but actually
//					works directly with the USART 'as is'
//					The ID-12 also has a CMOS data output (D1) 'non-inverted' (Normally Low)

// USART			Used to receive data from the ID-12 RFID reader device
//					Baud rate 9600, Asynchronous mode

//	I/O				RFID Module D0 is connected to the USART0 receive (RXD0)
//					RXD0 is the alternate function of PORT E, bit 0; set for input in application code

//					Port D connects to the on-board switches
//						Switch 0 clears the LCD display - use in between scans of cards
//						Switch 1 compares the card most recently read, with a pre-stored value 
//********************************************************************

// Port E bit 0 is used by the RFID reader
#define RFID_DDR DDRD
#define RFID_PORT PIND
#define TTL_DATA 0	// TTL (data is inverted)

// Constants for RFID code validation
#define RFID_ASCII_CR 0x0D		// ASCII carriage return
#define RFID_ASCII_LF 0x0A		// ASCII line feed
#define RFID_STX 0x02 			// Start of transmission
#define RFID_ETX 0x03 			// End of transmission
#define RFID_CHECKSUM0_Index 11	// 12th byte read from the RFID tag
#define RFID_CHECKSUM1_Index 12	// 13th byte read from the RFID tag
#define RFID_CODE_LENGTH 16		// Number of bytes in correctly formatted code
#define RFID_TAG_ID_LENGTH 10	// Length of RFID TAG-ID

char RFID_DATA_BUFFER[RFID_CODE_LENGTH];	// Raw data as read from RFID TAG (in ASCII format)
unsigned char RFID_BYTE_COUNT;				// Number of bytes received so far, Also data buffer length
	
char REFERENCE_TAG_ID_CORRECT[RFID_TAG_ID_LENGTH] =   {'2','9','0','0','9','3','D','B','8','E'}; // tag for lewis
char REFERENCE_TAG_ID_CORRECT1[RFID_TAG_ID_LENGTH] =  {'2','9','0','0','9','4','2','7','C','2'}; //tag for princess

void RFID_STORE_BYTE_IN_BUFFER(char RFID_DATA_BYTE)
{
	RFID_DATA_BUFFER[RFID_BYTE_COUNT] = RFID_DATA_BYTE;
	RFID_BYTE_COUNT++;
}

void RFID_READ_VALIDATE()	// Called when RFID byte read count >= 16
{
	if(RFID_CODE_LENGTH != RFID_BYTE_COUNT)	// Check code is 16 bytes long
	{
		RFID_ERROR();
		//PORTB = ~0xF1;	// Code too long
		return;
	}
	// Code is correct length, proceed with additional validation checks

	if(RFID_STX != RFID_DATA_BUFFER[0])		// Check first byte is STX
	{
		RFID_ERROR();
		PORTB = ~0xF2;	// Code incorrect first byte is NOT STX
		return;
	}

	if(RFID_ASCII_CR != RFID_DATA_BUFFER[13])		// Check 14th byte is CR
	{
		RFID_ERROR();
		//PORTB = ~0xF3;	// Code incorrect 14th byte is NOT CR
		return;
	}

	if(RFID_ASCII_LF != RFID_DATA_BUFFER[14])		// Check 15th byte is LF
	{
		RFID_ERROR();
		//PORTB = ~0xF4;	// Code incorrect 15th byte is NOT LF
		return;
	}

	if(RFID_ETX != RFID_DATA_BUFFER[15])		// Check 16th (last) byte is ETX
	{
		RFID_ERROR();
		//PORTB = ~0xF5;	// Code incorrect 16th (last) byte is NOT ETX
		return;
	}
	
		// ALSO - check the XOR checksum
		// I currently have only one Tag compatible with the RFID reader, and for this tag
		// ASCII data "", the XOR checksum comes out 1 bit wrong, whichever way it is calculated
		// Need more compatible tags before this behaviour can be investigated further, and then
		// a checksum checker can be codifed and added here. 

	
	lcd_Clear();			// Clear the LCD display
	lcd_SetCursor(0x00);	// Set cursor position to 1st line, 1st column
	lcd_WriteString("CHECKING CARD ....");
	lcd_SetCursor(0x40);		// Set cursor position to line 2, col 0
	lcd_WriteString("PRESS 1 ");
	setScannedCard2();
	lcd_Clear();
	ENABLE_INT1();

	//RFID_DISPLAY_TAG_ID_ON_LCD();
	//RFID_DISPLAY_TAG_CHECKSUM_ON_LCD();
}

void RFID_ERROR()
{	// Here when too many bytes received, or validation fails
	lcd_Clear();			// Clear the LCD display
	lcd_SetCursor(0x00);	// Set cursor position to 1st line, 1st column
	lcd_WriteString("ERROR");
	RFID_DISPLAY_TAG_FULL_DATA_ON_LCD();
}

void RFID_DISPLAY_TAG_FULL_DATA_ON_LCD()
{
	for(int iIndex = 0; iIndex < RFID_CODE_LENGTH; iIndex++)
	{	// Display the sixteen characters of the RFID code (Inc start character, TAG ID and Checksum, CF, LF and end character)
		lcd_SetCursor(0x40 + iIndex);	// Start from LCD cursor position 2nd line, 1st column
		lcd_WriteChar(RFID_DATA_BUFFER[iIndex]);
	}
}

void RFID_DISPLAY_TAG_ID_ON_LCD()
{
	for(int iIndex = 0; iIndex < RFID_TAG_ID_LENGTH; iIndex++)
	{	// Display the ten DATA characters of the TAG ID
		lcd_SetCursor(0x46 + iIndex);	// Start from LCD cursor position 2nd line, 7th column
		lcd_WriteChar(RFID_DATA_BUFFER[iIndex + 1 /*Skip the STX character*/]);
	}
}

void RFID_DISPLAY_TAG_CHECKSUM_ON_LCD()
{
	lcd_SetCursor(0x0E);	// Move the LCD cursor position to 1st line, 14th column
	lcd_WriteChar(RFID_DATA_BUFFER[RFID_CHECKSUM0_Index]);
	lcd_SetCursor(0x0F);	// Move the LCD cursor position to 1st line, 15th column
	lcd_WriteChar(RFID_DATA_BUFFER[RFID_CHECKSUM1_Index]);
}

