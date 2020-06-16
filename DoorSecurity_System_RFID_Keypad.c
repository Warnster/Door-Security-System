/*
 * DoorSecurity_System_RFID_Keypad_.c
 *
 * Created: 11/25/2016 10:15:52 AM
 *  Author: Student
 */ 


// Project 		RFID Reader with LCD output
// Target 		ATmega1281 on STK300


//	Port configurations
//					Port A bits 0-7 used by the LCD Display
//					Port B bits Servo
//					Port C bits 6-7 used by the LCD Display
//					Port E bit 0 (USART RXD) is used by the RFID reader
//					Port D bit 0 (Switches connected to H/W INT0 and INT1)
//					Port G bits 0-1 used by the LCD Display
//********************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "LCD_LibraryFunctions_1281.h"
#include "RFID_Reader_1281.h"
#include "USART0_Configuration_For_RFID_1281.h"

//keypad code 
#define star 0xF6
#define hash 0xA4
#define zero 0xE9
#define noKey 0xFF

#define scanKeypadRow0 0b00111111	// Bits 4-7 pulled low depending on row being scanned, bits 0-2 (pullups) remain high at all times
#define scanKeypadRow1 0b01011111
#define scanKeypadRow2 0b01101111
#define scanKeypadRow3 0b01110111

#define keypadMaskColumns 0b11111000
#define keypadMaskColumn0 0b00000100
#define keypadMaskColumn1 0b00000010
#define keypadMaskColumn2 0b00000001
#define PinCodeLength 3

void InitialiseGeneral(void);
void InitialiseTimer3_Servo_Timer(void);
void Initialise_INT0();		// PORT D - Switch 0 to reset reader system and clear LCD
void Initialise_INT1();		// PORT D - Switch 1 to compare RFID TAG ID read with pre-stored reference TAG ID
void InitialiseTimer1();
//void InitialiseTimer2();
int keyPressedAmount =0;

//void InitialiseGeneral(void);
unsigned char ScanKeyPad(void);
unsigned char ScanColumns(unsigned char);
void DisplayKeyValue(unsigned char);
void DebounceDelay(void);
int correctKey;
int code = 0;
unsigned char keyCode[PinCodeLength];
unsigned char tempKeyCode[PinCodeLength];
unsigned char keyValue;
// end
int ElapsedSeconds_Count = 0;
int count = 1;
unsigned buzz_value;
unsigned wrong_value;
int cardScanned;
int iMatch;
int correctNumberNotEntered = 0;
char LCD_UPDATED = 0;
int cardScanned2;
int main(void)
{	
	DISABLE_INT1();				//disables interrupt 1 until int 2 is triggered
	RFID_BYTE_COUNT = 0;
	char LCD_UPDATED = 0;
	cardScanned = 0;
	cardScanned2 = 0;
	iMatch = 0;
	InitialiseGeneral();		//sets the ports and pins and global variables
	InitialiseTimer3_Servo_Timer();//for the servo hardware timer
	Initialise_INT1();			//sets the interrupt 1
	InitialiseTimer1();			//initialises hardware timer 1
	USART_SETUP_9600_BAUD_ASSUME_1MHz_CLOCK();
	lcd_Clear();				// Clear the display
	lcd_StandardMode();			// Set Standard display mode
	lcd_on();					// Set the display on
	lcd_CursorOff();			// Set the cursor display off (underscore)
	lcd_CursorPositionOff();	// Set the cursor position indicator off (flashing square)

	//does the loop until cardScanned !=0 when int1 is triggered.
	while (cardScanned == 0) {
		//if cardScanned2 ==0 display user to swipe card, else displays enter 1 to trigger interrupt1
		if(cardScanned2 == 0) {
			
		lcd_SetCursor(0x00);		// Set cursor position to line 1, col 0
		lcd_WriteString("SCAN CARD");
		lcd_SetCursor(0x40);		// Set cursor position to line 2, col 0
		lcd_WriteString("Awaiting Input");
		}
		else if(cardScanned2 == 1) {
			lcd_SetCursor(0x00);	// Set cursor position to 1st line, 1st column
			lcd_WriteString("CHECKING CARD ....");
			lcd_SetCursor(0x40);		// Set cursor position to line 2, col 0
			lcd_WriteString("PRESS 1 ");
		}
		if(cardScanned == 1) {
			break;
		}
	}
	//checks if the cards are matching the reset cards
	if (	RFID_DATA_BUFFER[RFID_TAG_ID_LENGTH] != REFERENCE_TAG_ID_CORRECT[RFID_TAG_ID_LENGTH] & RFID_DATA_BUFFER[RFID_TAG_ID_LENGTH] != REFERENCE_TAG_ID_CORRECT1[RFID_TAG_ID_LENGTH] ) {
		iMatch = 0;
	}
	//if card is a match enables 5 second count on timer 1 
	if(1 == iMatch)
	{
		TCCR1B = 0b00001101;	//start timer 1
		lcd_Clear();			// Clear the LCD display
		lcd_SetCursor(0x00);	// Set cursor position to 1st line, 1st column
		
		lcd_WriteString("ENTER PIN: ");
		//polls the keypad for input and checks for correct input
		//code = 1 if correct
		pollKeyPad();
		//
		if(code == 1) {
			correctNumberNotEntered = 0;
			//displays a message based on what card was swiped
			displaySpecificCardMessage();
			//opens the door (servo)
			openAndCloseDoor();
			//resets code and returns to main
			code = 0;
			return main();
		}
		
	}
	
	else
	{
		//if wrong code is entered displays invalid card and buzzes 3 times.
		lcd_Clear();
		lcd_WriteString("INVALID CARD");
		wrong_buzzer();
		_delay_ms(1000);
		wrong_buzzer();
		_delay_ms(1000);
		wrong_buzzer();
		_delay_ms(1000);
		return main();
	}
	
}
//this method is called from the RFID-reader_1281 to set a variable when int2 happens
void setScannedCard2() {
	cardScanned2 = 1;
}
//method loops until the user has entered 4 number from keypad. 
void pollKeyPad() {
	while(keyPressedAmount !=4) {
		keyValue = ScanKeyPad();
		if(noKey != keyValue) {
			DisplayKeyValue(keyValue);
			DebounceDelay();
		}
	}
	//resets keyPressedAmount to 0
	keyPressedAmount =0;
}

void InitialiseGeneral()
{ 
	 //Ports for servos	 
	
	DDRE = 0xFF;		// Port E bit 3 must be set as OUTPUT to provide the PWM pulse on OC3A
					    // Port E bit 4 must be set as OUTPUT to provide the PWM pulse on OC3B
					    // Port E bit 5 must be set as OUTPUT to provide the PWM pulse on OC3C
					    // Port E bit 7 Input Capture 3 Pin (ICP3) must be set as OUTPUT to prevent 
					    // random / noise values entering ICR3 (ICR3 used as TOP value for PWM counter)
	PORTE = 0x00;

	// Ports for LCD
	DDRA = 0xFF;		// Set port A (Data and Command) sas output
	DDRC = 0b11001000;		// Set port C (bits 6 and 7 for RS and ENABLE) as output
	PORTC = 0x00;		// RS and ENABLE initially low
	DDRG = 0x3F;		// Set port G (bits 0 and 1 for RD and WR) as output
	
	//port for push button
	//Port B used for keypad. 
	DDRB = 0xF8;	
	PORTB = 0x07;
	//port for rfid
	DDRD = 0b00001000;		// Port E bit 0 (read data from RFID reader set for input)
	
	buzz_value =0; 
	wrong_value =0;
	correctNumberNotEntered = 0;
	//keypad variables
	//Sets the keypad Security door code 1234
	keyCode[0] = 1;
	keyCode[1] = 2;
	keyCode[2] = 3;
	keyCode[3] = 4;
	//sets temp code to 0000
	for(int i=0;i<4;i++) {
		tempKeyCode[i] = 0;
	}
	sei();				// Enable interrupts at global level, set Global Interrupt Enable (I) bit
}

void displaySpecificCardMessage() {
	//displays a message to the LCD depending on what card has been scanned. returns to main if invalid card scanned
	if(	RFID_DATA_BUFFER[RFID_TAG_ID_LENGTH] == REFERENCE_TAG_ID_CORRECT[RFID_TAG_ID_LENGTH])
	{
		lcd_WriteString("Welcome Lewis");
	}
	else if (RFID_DATA_BUFFER[RFID_TAG_ID_LENGTH] == REFERENCE_TAG_ID_CORRECT1[RFID_TAG_ID_LENGTH])
	{
		lcd_WriteString("Welcome Princess");
	}
	else if (	RFID_DATA_BUFFER[RFID_TAG_ID_LENGTH] != REFERENCE_TAG_ID_CORRECT[RFID_TAG_ID_LENGTH] & RFID_DATA_BUFFER[RFID_TAG_ID_LENGTH] != REFERENCE_TAG_ID_CORRECT1[RFID_TAG_ID_LENGTH] )  {
		lcd_WriteString("INVALID CARD");
		wrong_buzzer();
		_delay_ms(1000);
		wrong_buzzer();
		_delay_ms(1000);
		wrong_buzzer();
		_delay_ms(1000);
		return main();
	}
}

void Initialise_INT1()		// PORT D - Switch 1 to compare RFID TAG ID read with pre-stored reference TAG ID
{
	
	EICRA = 0b00001000;		// Interrupt Sense (INT0) falling-edge triggered	
	EICRB = 0b00000000;	
	EIMSK = 0b00000000;	// Set bit 1 to Enable H/W Int 1	
	EIFR = 0b11111111;		// Clear all HW interrupt flags (in case a spurious interrupt has occurred during chip startup)
}

void ENABLE_INT1()
{
	EIMSK = 0b00000010;		// Enable H/W Int 1
}

void DISABLE_INT1()
{
	EIMSK = 0b00000000;		// Disable H/W Int 1
}


//***** Hardware Interrupt Handlers

ISR(INT1_vect)
{
	//sets elapsedseconds to 0 for accurate timing of timer 1
	ElapsedSeconds_Count = 0;
	correctNumberNotEntered = 1;
	cardScanned = 1;
	RFID_TAG_COMPARE(); // Compare the RFID tag against the reference tag ID
	
	RFID_BYTE_COUNT = 0;
}




void RFID_TAG_COMPARE()
{	// Compare the 10-byte TAG ID of the most-recently read tag in DATA memory, with
	// a statically-coded reference TAG ID
	
	
	
	for(int iIndex = 0; iIndex < RFID_TAG_ID_LENGTH; iIndex++)
	{
		if(	RFID_DATA_BUFFER[iIndex + 1 /*Skip the STX character*/] == REFERENCE_TAG_ID_CORRECT[iIndex] & REFERENCE_TAG_ID_CORRECT1[iIndex])
		{
			iMatch = 1;
		}
	}
	
	
}

void InitialiseTimer3_Servo_Timer()
{
	
	TCCR3A = 0b10000010;	// Fast PWM non inverting, ICR3 used as TOP
	TCCR3B = 0b00000001;	// Fast PWM, Use Prescaler '1'
	TCCR3C = 0b00000000;

	ICR3H = 0x46; // 16-bit access (write high byte first, read low byte first)
	ICR3L = 0x50;

	// Set Timer/Counter count/value registers (16 bit) TCNT1H and TCNT1L
	TCNT3H = 0; // 16-bit access (write high byte first, read low byte first)
	TCNT3L = 0;
	
	// 'Neutral' (Mid range) pulse width 1.5mS = 1500uS pulse width
	OCR3A = 1500;
	
	TIMSK3 = 0b00000000;	// No interrupts needed, PWM pulses appears directly on OC3A, OC3B, OC3C (Port E Bits 3,4,5)
	
	// TIFR3 – Timer/Counter3 Interrupt Flag Register
	TIFR3 = 0b00101111;		// Clear all interrupt flags
}

void openAndCloseDoor() {
	//calls open door and close door methods which send a pwm to the servo
	lcd_Clear();
	lcd_WriteString("DOOR OPENING");
	door_buzzer();
	open_door();
	_delay_ms(2000);
	close_door();
}

void InitialiseTimer1()	// Configure to generate an interrupt after a 1-Second interval
{
	TCCR1A = 0b00000000;	// Normal port operation (OC1A, OC1B, OC1C), Clear Timer on 'Compare Match' (CTC) waveform mode)
	TCCR1B = 0b00001000;	// CTC waveform mode, use prescaler 1024
	TCCR1C = 0b00000000;
	
	// For 1 MHz clock (with 1024 prescaler) to achieve a 1 second interval:
	// Need to count 1 million clock cycles (but already divided by 1024)
	// So actually need to count to (1000000 / 1024 =) 976 decimal, = 3D0 Hex
	OCR1AH = 0x03; // Output Compare Registers (16 bit) OCR1BH and OCR1BL
	OCR1AL = 0xD0;

	TCNT1H = 0b00000000;	// Timer/Counter count/value registers (16 bit) TCNT1H and TCNT1L
	TCNT1L = 0b00000000;
	TIMSK1 = 0b00000010;	// bit 1 OCIE1A		Use 'Output Compare A Match' Interrupt, i.e. generate an interrupt
	// when the timer reaches the set value (in the OCR1A register)
}

void startTimer() {
	//starts the timer 1 for the 5 second count
	ElapsedSeconds_Count = 0;
	TCCR1B = 0b00001101;
	lcd_Clear();
	
}

void open_door()
{	
	//TCCR3B = 0b00011001;
	
	 OCR3A = 1500;	
	 _delay_ms(2000);
	 	 			
}

void close_door()
{
	//TCCR3B = 0b00011001;
	OCR3A -= 1500;
	_delay_ms(2000);
}

//method to sound correct code opening
//smaller delay between bits changing creates higher pitch
void door_buzzer(int ms)
{
	int i;
	for(i=0;i<50;i++)
	{
			PORTD=0b00000000;
			_delay_ms(3);
			PORTD=0b00001000;
			_delay_ms(3);
					
	}
	for(i=0;i<50;i++)
	{
		PORTD=0b00000000;
		_delay_ms(1);
		PORTD=0b00001000;
		_delay_ms(1);
		
	}
}
//sounds incorrect buzzer
void wrong_buzzer()
{
	int i;
	for(i=0;i<80;i++)
	{
		PORTD=0b00000000;
		_delay_ms(2);
		PORTD=0b00001000;
		_delay_ms(2);
		
	}

}

//keypad method scans for input and returns key value entered by user
unsigned char ScanKeyPad() {
	unsigned char rowWeight;
	unsigned char keyValue;
	
	
	rowWeight = 0x01;
	PORTB = scanKeypadRow0;
	keyValue = ScanColumns(rowWeight);
	if(noKey != keyValue) {
		return keyValue;
	}
	rowWeight = 0x04;
	PORTB = scanKeypadRow1;
	keyValue = ScanColumns(rowWeight);
	if(noKey != keyValue) {
		return keyValue;
	}
	rowWeight = 0x07;
	PORTB = scanKeypadRow2;
	keyValue = ScanColumns(rowWeight);
	if(noKey != keyValue) {
		return keyValue;
	}
	rowWeight = 0x0A;
	PORTB = scanKeypadRow3;
	keyValue = ScanColumns(rowWeight);
	
		return keyValue;
	
}
//checks what column had been pressed
unsigned char ScanColumns(unsigned char rowWeight) {
	unsigned char columnPinValue;
	columnPinValue = PINB | keypadMaskColumns;
	columnPinValue = ~columnPinValue;
	switch(columnPinValue) {
		case keypadMaskColumn0: 
			return rowWeight;
		case keypadMaskColumn1:
			return rowWeight + 1;
		case keypadMaskColumn2:
			return rowWeight + 2;
		default:
			return noKey;
	}
	
}
//displays astrix for every input entered
void DisplayKeyValue(unsigned char keyValue) {
	//clears the keypad entry
	switch(keyValue) {
		case 0x0A:
			lcd_Clear();
			lcd_WriteString("Enter Pin:");
			keyPressedAmount = 0;
			return;
		
			
	}
	//outputs astrix to lcd, changes cursor position depending on keypressed amount
	if(keyValue != 0x00 && keyPressedAmount != 4) {
		if(keyPressedAmount == 0){
			lcd_SetCursor(0x0A);
			lcd_WriteChar(0x2A);
			
			} else if(keyPressedAmount == 1) {
			lcd_SetCursor(0x0B);
			lcd_WriteChar(0x2A);
			} else if(keyPressedAmount == 2) {
			lcd_SetCursor(0x0C);
			lcd_WriteChar(0x2A);
			} else if(keyPressedAmount == 3) {
			lcd_SetCursor(0x0D);
			lcd_WriteChar(0x2A);
			//on final key pressed calls the security check to compare the code entered to the correct code
			securityCheck();
		}
		//adds keyvalue to tempcode array
		constructCode(keyValue);
		
		keyPressedAmount++;
	} else {
		lcd_Clear();
		lcd_StandardMode();			// Set Standard display mode
		lcd_on();					// Set the display on
		lcd_CursorOff();			// Set the cursor display off (underscore)
		lcd_CursorPositionOff();	// Set the cursor position indicator off (flashing square)
	}
	
	
	keyValue = 0x00;
	
}
//sets correct key to 1 if all array values are equal
void securityCheck() {
	for(int i = 0; i<4; i++) {
		if(keyCode[i] == tempKeyCode[i]) {
			correctKey = 1;
		} else {
			correctKey = 0;
			break;
		}
	}
	
}

void constructCode(char keyValue) {
	//enters the number in the tempKeyCode[keyPressedAmount]
	switch (keyValue) {
		case 0x01:
			tempKeyCode[keyPressedAmount] = 0x01;
		break;
		case 0x02:
			tempKeyCode[keyPressedAmount] = 0x02;
			break;
		case 0x03:
			tempKeyCode[keyPressedAmount] = 0x03;
			break;
		case 0x04:
			tempKeyCode[keyPressedAmount] = 0x04;
			break;
		case 0x05:
			tempKeyCode[keyPressedAmount] = 0x05;
			break;
		case 0x06:
			tempKeyCode[keyPressedAmount] = 0x06;
			break;
		case 0x07:
			tempKeyCode[keyPressedAmount] = 0x07;
			break;
		case 0x08:
			tempKeyCode[keyPressedAmount] = 0x08;
			break;
		case 0x09:
			tempKeyCode[keyPressedAmount] = 0x09;
			break;
	}
	
	if(keyPressedAmount == PinCodeLength) {
		//checks code
		securityCheck();
		
		if(code == 1) {
			lcd_Clear();
			
		} else {
			lcd_Clear();
			lcd_WriteString("WRONG PIN");
			wrong_buzzer();
			_delay_ms(1000);
			wrong_buzzer();
			_delay_ms(1000);
			wrong_buzzer();
			keyPressedAmount = 0;
			return main();
		}
		
	}
}

void DebounceDelay() { // debounce delay for mechanical key presses
	for(int i = 0; i < 50; i++)
	{
		for(int j = 0; j < 255; j++);
	}
}

ISR(TIMER1_COMPA_vect) // TIMER1_CompareA_Handler (Interrupt Handler for Timer 1)
{	// Flip the value of the least significant bit of the 8-bit variable
	ElapsedSeconds_Count++;
	//if 5 sseconds is reached stops and the correct number is not entered then the system resets.
	//else the timer is stoped and the program continues
	if(ElapsedSeconds_Count == 5 & correctNumberNotEntered == 1) {
		TCCR1B = 0b00001000;	// Stop the timer (CTC, no clock)
		lcd_Clear();
		lcd_WriteString("Scanner Reset");
	keyPressedAmount = 0;
		_delay_ms(2000);
		
		return main();
		 
	} else if(ElapsedSeconds_Count == 5 & correctNumberNotEntered == 0) {
		TCCR1B = 0b00001000;
	}
}