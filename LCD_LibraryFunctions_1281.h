//*******************************************************************************
// Project 		LCD Library Functions header (Embedded C) 
// Target 		ATMEL ATmega1281 micro-controller on STK300 board
// Program		LCD_LibraryFunctions_1281.h
// Author		Richard Anthony
// Date			11th October 2013 (8535 version 8th October 2011)

// Function		Provides low-level functions to configure the LCD

// Ports	  	Uses the 14-pin SIL LCD interface connection on the STK300

// 				Uses port A for Data and Command values (8 bits, Output)
//				The LCD 'device Busy' flag also uses bit 7 of port A 
//				So sometimes this bit has to be set for Input

//				Uses port C for control (register select 'RS' bit 6 Output)
//										(device enable 'ENABLE' bit 7 Output)

//				Uses port G for control (write 'WR' bit 0 Output) 
//										(read 'RD' bit 1 Output) 
//*******************************************************************************

//Function declarations
void lcd_wait();				//Check if the LCD device is busy, if so wait.
void lcd_WriteFunctionCommand();	//Output the function command in LCDreg to the LCD:
void lcd_ReadFunctionCommand();		//Read the function command from the LCD into LCDreg
void lcd_Clear();					//Clear the LCD display and set the cursor to the 'home' position
void lcd_StandardMode();			//Set LCD to 8-bit-mode, display window freeze and auto cursor increment
void lcd_SetCursor(unsigned char cursorPosition);	//Set cursor to a specific display position
void lcd_WriteChar(char cValue);			//Write the char in LCDreg at current cursor position and increment cursor
void lcd_WriteString(char []);
void lcd_on();						//Set LCD display on, cursor on and blink on
void lcd_CursorOn();				//Set Cursor on
void lcd_CursorOff();				//Set Cursor Off
void lcd_DisplayOn();				//Set Display on
void lcd_DisplayOff();				//Set Display off
void lcd_CursorPositionOff();		//Set Cursor Position Indicator Off
void lcd_BarGraph(unsigned char bar1, unsigned char bar2);	//Display 2-bar bar graph on LCD display
void lcd_ShiftLeft();	// shift LCD display one place to the left
void lcd_ShiftRight();	// shift LCD display one place to the right
void lcd_OneLineMode();	// configure LCD one-line mode
void lcd_TwoLineMode();	// configure LCD two-line mode
void lcd_WriteVariable_withValueAndPositionParameters(unsigned char row, unsigned char column, unsigned char data); // Display a variable IN RAW format at a user-specified position
void lcd_WriteVariable_withValueAndPositionParameters_SingleDecimalDigit(unsigned char row, unsigned char column, unsigned char data); // Display a single-digit DECIMAL value (0-9)

// Declare global variables required for the LCD library
unsigned char  LCDreg;

// PORTA control bits
//	BF=7
// PORTC control bits
//	RS=6
//	ENABLE=7
// PORTG control bits  (for 1281)
//	WR=0
//	RD=1

void lcd_Wait()		// Check if the LCD device is busy, if so wait
{	
	PORTC &= 0b00111111; // Clear enable (bit 7), clear Register select (bit 6) 
						 // (Set RS low (command register) so can read busy flag)
	PORTG &= 0b11111101; // Clear RD (bit 1) for read operation
	PORTG |= 0b00000001; // Set WR (bit 0) for read operation
	DDRA &= 0b01111111;  // High bit of port A is Busy Flag (BF, bit 7), set to input
	PORTC |= 0b10000000; // Set enable (bit 7)

	while(PINA & 0b10000000); // Wait here until busy flag is cleared

	PORTC &= 0b01111111; // Clear enable (bit 7)
	PORTG &= 0b11111110; // Clear WR (bit 0) for write operation
	PORTG |= 0b00000010; // Set RD (bit 1) for write operation
	DDRA |= 0b10000000;  // Restore bit 7 of port A to output (for Data use)
}

void lcd_WriteFunctionCommand()	// Output the function command in LCDreg to the LCD
{
	lcd_Wait(); 		// Wait if the LCD device is busy
	PORTC &= 0b10111111; // Clear Register select (bit 6) 
	PORTC |= 0b10000000; // Set enable (bit 7)
	PORTA = LCDreg;	// Send function command to command register, via port A
	PORTC &= 0b01111111; // Clear enable (bit 7)
}

void lcd_ReadFunctionCommand()	// Read the function command from the LCD into LCDreg
{	
	lcd_Wait(); 		// Wait if the LCD device is busy
	PORTC &= 0b10111111; // Clear Register select (bit 6)
	PORTC |= 0b10000000; // Set enable (bit 7)
	LCDreg = PORTA;	// Read the command register via port A
	PORTC &= 0b01111111; // Clear enable (bit 7)
}

void lcd_Clear()	// Clear the LCD display and set the cursor to the 'home' position
{	
	LCDreg = 0x01; 				// The 'clear' command
	lcd_WriteFunctionCommand(); // Output the command to the LCD
}

void lcd_StandardMode()	// Set the LCD to 8-bit-mode, display window freeze and
						// automatic cursor increment (standard mode)
{
	LCDreg = 0b00111000; 			// Command for 8-Bit-transfer
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
	LCDreg = 0b00000110; 			// Command for Increment, display freeze
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
	LCDreg = 0b00010000; 			// Command for Cursor move, not shift
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
}

void lcd_SetCursor(unsigned char cursorPosition)	// Set cursor on the LCD to a specific display position
{
	LCDreg = cursorPosition | 0b10000000; 	// Set bit 7 of the byte
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
}

void lcd_WriteChar(char cValue)	// Write the character in LCDreg to the display at the current cursor
											// position (the position is incremented after write)	
{
	lcd_Wait(); 		// Wait if the LCD device is busy
	PORTC |= 0b11000000;// Set enable (bit 7), Set RS (bit 6) (for data)
	PORTA = cValue;  	// Write data character to LCD
	PORTC &= 0b01111111;// Clear enable (bit 7)
}

void lcd_WriteString(char Text[])
{
	unsigned char Index;
	for(Index = 0;Index < strlen(Text); Index++)
	{
		lcd_WriteChar(Text[Index]);
	}
}
 
void lcd_on()	// Set LCD display on, cursor on and blink on
{
	LCDreg = 0b00001111; 			// Combined command byte
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
}

void lcd_CursorOn()// Set Cursor on	
{
	lcd_ReadFunctionCommand();	// Input the command register value into the LCDreg
	LCDreg |= 0b00000010; 		// Set bit 1 of the position. Command value to set cursor on
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
}

void lcd_CursorOff()	// Set Cursor Off	
{
	lcd_ReadFunctionCommand();	// Input the command register value into the LCDreg
	LCDreg &= 0b11111101;		// Command value to set cursor off
	lcd_WriteFunctionCommand(); // Output the command to the LCD
}

void lcd_DisplayOn()	// Set Display on
{
	lcd_ReadFunctionCommand();	// Input the command register value into the LCDreg
	LCDreg |= 0b00000100; 		// Set bit 2 of the position. Command value to set display on
	lcd_WriteFunctionCommand(); // Output the command to the LCD
}

void lcd_DisplayOff()	// Set Display off
{
	lcd_ReadFunctionCommand();	// Input the command register value into the LCDreg
	LCDreg &= 0b11111011;		// Command value to set display off
	lcd_WriteFunctionCommand();	// Output the command to the LCD
}

void lcd_CursorPositionOff()	// Set Cursor Position Indicator Off
{
	LCDreg = 0b00001100; 			// Command value to set cursor position indicator off
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
}

void lcd_BarGraph(unsigned char bar1, unsigned char bar2) 
// Display 2-bar bar graph on LCD display
// The bar lengths are given by the values in Bar1 and Bar2
{
	unsigned char  BarGraph1 = bar1;
	unsigned char  BarGraph2 = bar2;
	lcd_CursorOff();	
	lcd_CursorPositionOff();	
	lcd_Clear();
	lcd_SetCursor(0x00);   // Set cursor position to line 1, col 0

	while(BarGraph1 > 0)	// Check for 0 bar height
	{
		lcd_WriteChar(0x11); // 0x11 is the best character code for the bar element
		BarGraph1--;
	}

	lcd_SetCursor(0x40);   // Set cursor position to line 2, col 0

	while(BarGraph2 > 0)	// Check for 0 bar height
	{
		lcd_WriteChar(0x11); // 0x11 is the best character code for the bar element
		BarGraph2--;
	}
}

void lcd_ShiftLeft()		// Configure LCD shift Left mode
{
	LCDreg = 0b00011000; 			// Command value to set shift Left (see LCD manual PDF)
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
}
	
void lcd_ShiftRight()	// Configure LCD shift Right mode
{
	LCDreg = 0b00011100; 			// Command value to set shift Right (see LCD manual PDF)
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
}

void lcd_OneLineMode()	// Configure LCD one-line mode
{
	LCDreg = 0b00110100; 			// Command value to set to one-line mode (see LCD manual PDF)
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
}

void lcd_TwoLineMode()	// Configure LCD Two-line mode
{
	LCDreg = 0b00111100;			// Command value to set to two-line mode (see LCD manual PDF)
	lcd_WriteFunctionCommand(); 	// Output the command to the LCD
}

void lcd_WriteVariable_withValueAndPositionParameters(unsigned char row, unsigned char column, unsigned char data)
{
	// The LCD has 2 rows R = {0 - 1} and 16 columns C = {0 - 15}
	// The cursor position is defined in a single byte as follows: 0b0B00CCCC, so
	// need to mask and shift the passed-in values, before calling lcd_SetCursor()
	column &= 0b00001111;	// Only lower 4 bits are valid, Mask off upper 4 bits
	row &= 0b00000001;		// Only lowest bit is valid, Mask off upper 7 bits
	row *= 64;				// Shift row value 6 places to the left
	lcd_SetCursor(column + row);	// Set the cursor position
	lcd_WriteChar(data);	// Write the data byte to the display at current cursor position
}

void lcd_WriteVariable_withValueAndPositionParameters_SingleDecimalDigit(unsigned char row, unsigned char column, unsigned char data)
{
	// The LCD has 2 rows R = {0 - 1} and 16 columns C = {0 - 15}
	// The cursor position is defined in a single byte as follows: 0b0R00CCCC, so
	// need to mask and shift the passed-in values, before calling lcd_SetCursor()
	column &= 0b00001111;	// Only lower 4 bits are valid, Mask off upper 4 bits
	row &= 0b00000001;		// Only lowest bit is valid, Mask off upper 7 bits
	row *= 64;				// Shift row value 6 places to the left
	lcd_SetCursor(column + row);	// Set the cursor position
	data += '0';			// Add ASCII '0' to the data value (convert a decimal number into its character representation)
	lcd_WriteChar(data);	// Write the data byte to the display at current cursor position
}