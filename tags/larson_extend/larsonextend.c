/*
larsonextend.c
The Larson Scanner -- Alternative version to allow scanner to run off the edge of the board.
 
 It simulates one LED at brightness 4, followed by one LED of brightness 1, that moves across
 the nine pixels, disappearing off either end of the board, before returning to scan in the other direction.
 
 There is no longer any overlap of these "LEDs" at either end, but up to three LEDs may be lit at a time as
 the head fades in and the tail fades out.
 
 Also, some of the input and output values and pull-up resistors have been changed from the original program 
 in anticipation of future extensibility.
 
 With the buttons linked between the units, it seems to be flakey, at best, to get an accurate button press
 on multiple units at the same time, so that section is commented out below.  Change the speed in the variable
 declarations as you see fit.  Also, since this program was originally written for a specific application
 (a permanent installation in an enclosure), the unit was never really intended to change speeds.  I tried to
 get it to work like the original, but couldn't.  Sorry, guys.

Original written by Windell Oskay, http://www.evilmadscientist.com/
New alternative version written by John Breen III
 
 Copyright 2009 Windell H. Oskay, 2010 John J. Breen III
 Distributed under the terms of the GNU General Public License, please see below.
 
 

 An avr-gcc program for the Atmel ATTiny2313  
 
 Based on Version 1.1_alt1, written by Windell Oskay
 
 Version 1.1   Last Modified:  26-Sep-2010. 
 Written for Evil Mad Science Larson Scanner Kit, based on the "ix" circuit board. 
 
 
 More information about this project is at 
 http://www.evilmadscientist.com/article.php/larsonkit
 
 
 
 -------------------------------------------------
 USAGE: How to compile and install
 
 
 
 A makefile is provided to compile and install this program using AVR-GCC and avrdude.
 
 To use it, follow these steps:
 1. Update the header of the makefile as needed to reflect the type of AVR programmer that you use.
 2. Open a terminal window and move into the directory with this file and the makefile.  
 3. At the terminal enter
 make clean   <return>
 make all     <return>
 make install <return>
 4. Make sure that avrdude does not report any errors.  If all goes well, the last few lines output by avrdude
 should look something like this:
 
 avrdude: verifying ...
 avrdude: XXXX bytes of flash verified
 
 avrdude: safemode: lfuse reads as 62
 avrdude: safemode: hfuse reads as DF
 avrdude: safemode: efuse reads as FF
 avrdude: safemode: Fuses OK
 
 avrdude done.  Thank you.
 
 
 If you a different programming environment, make sure that you copy over the fuse settings from the makefile.
 
 
 -------------------------------------------------
 
 This code should be relatively straightforward, so not much documentation is provided.  If you'd like to ask 
 questions, suggest improvements, or report success, please use the evilmadscientist forum:
 http://www.evilmadscientist.com/forum/
 
 
 -------------------------------------------------
 
  
*/

#include <avr/io.h> 

#define shortdelay(); 			asm("nop\n\t" \
"nop\n\t");

  
int main (void)
{
uint8_t LEDs[9]; // Storage for current LED values
uint8_t rightLED[6], leftLED[6]; // Storage for initialization visualization LEDs
	
int8_t eyeLoc[5]; // List of which LED has each role, leading edge through tail.

uint8_t LEDBright[4] = {4U,2U,1U,1U};   // Relative brightness of scanning eye positions, head through tail

void delay_ms(uint8_t ms) {
   uint16_t delay_count = 100;
   volatile uint16_t i;
 
   while (ms != 0) {
     for (i=0; i != delay_count; i++);
     ms--;
   }
 }
	
int8_t j, k, m;
	
uint8_t position, loopcount, direction, initloopcount, already_running, softbounce;
uint8_t runitout, d_base, a_base, d_mod, a_mod, far_left, far_right;
uint8_t ILED, RLED, MLED;	// Eye position variables: Integer, Modulo, remainder

uint8_t delaytime;

uint8_t  pt, debounce, speedLevel;
unsigned int debounce2, BrightMode;
	 
uint8_t LED0, LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8;
	
//Initialization routine: Clear watchdog timer-- this can prevent several things from going wrong.		
MCUSR &= 0xF7;		//Clear WDRF Flag
WDTCSR	= 0x18;		//Set stupid bits so we can clear timer...
WDTCSR	= 0x00;

//Data direction register: DDR's
//Port A: 1 is an output, A0 is an input.	
//Port B: 0-3 are outputs, B4 is an input.	
//Port D: 1-6 are outputs, D0 is an input.
	
	DDRA = 2U;
	DDRB = 15U;	
	DDRD = 126U;
	
	a_base = 3; // set a base value (resting value) for PA, to keep things easy to modify
	d_base = 3; // set a base value (resting value) for PD, to keep things easy to modify
	
	PORTA = a_base;	// Pull-up resistor enabled, PA0, Port A1 High 
	PORTB = 16;	// Pull-up resistor enabled, PB4
	PORTD = d_base;  // Pull-up resistor enabled, PD0, Port D1 High
	
	d_mod = 0;
	a_mod = 0;
	
/* Visualize outputs:
 
 L to R:
 
 D2 D3 D4 D5 D6 B0 B1 B2 B3	
 
 <-- A1 (out to left)
 
	  (out to right) D1 -->
  
*/

// Clear out all of the LED values to blank out the display

	j = 0;
	while (j < 9) {
		LEDs[j] = 0; 
		j++;
	} 

	 LED0 = LEDs[0];
     LED1 = LEDs[1];
	 LED2 = LEDs[2];
	 LED3 = LEDs[3];
	 LED4 = LEDs[4];
	 LED5 = LEDs[5];
	 LED6 = LEDs[6];
	 LED7 = LEDs[7];
	 LED8 = LEDs[8]; 
	
//multiplexed = LowPower;	
 
	debounce = 1; 
	debounce2 = 1;
	loopcount = 254;
	initloopcount = 5; 
	delaytime = 0;
	
	direction = 0;
	position = 0;
	runitout = 0;
	already_running = 0;
	softbounce = 0;
	far_left = 0;
	far_right = 0;
	speedLevel = 3;  // Range: 1, 2, 3
	BrightMode = 0;

if ((PINB & 16) == 0)		// Check if button held on startup; used to verify wiring configuration
	{ initloopcount = 0; // if so, set the startup loop counter to 0 so that we can watch the startup lights
	  softbounce = 1; // Also set the "soft-bounce" mode, which bounces back without leaving the edge, like the original
	}	

delay_ms(200);

PORTD = 1; //Pull D1 low, to trigger output on right side

delay_ms(10); //Delay to allow output to latch

if ((PIND & 1) == 0) //Check to see if D1 output has latched D0 input
	far_right = 1; //If D0 and D1 are connected, we're at the end of the chain; set far_right so we bounce back from this end

PORTD = d_base; //Set D1 high

PORTA = 1; //Pull A1 low, to trigger output on left side

delay_ms(10); //Delay to allow output to latch

if ((PINA & 1) == 0) //Check to see if A1 output has latched A0 input
	far_left = 1; //If A0 and A1 are connected, we're at the end of the chain; set far_left so we bounce back from this end

PORTA = a_base; //Set A1 high


//A little bit if visual verification to the user as to which way each end of the scanner is set
// (flash outwards if it's open-ended to go to the next scanner in the chain, flash inwards if it's the end and will bounce back

if (far_left) {
	leftLED[0] = 7;
	leftLED[1] = 11;
	leftLED[2] = 19;
	leftLED[3] = 19;
	leftLED[4] = 19;
	leftLED[5] = 19;
}
else {
	leftLED[0] = 19;
	leftLED[1] = 11;
	leftLED[2] = 7;
	leftLED[3] = 7;
	leftLED[4] = 7;
	leftLED[5] = 7;
}

if (far_right) {
	rightLED[0] = 24;
	rightLED[1] = 20;
	rightLED[2] = 18;
	rightLED[3] = 18;
	rightLED[4] = 18;
	rightLED[5] = 18;
}
else {
	rightLED[0] = 18;
	rightLED[1] = 20;
	rightLED[2] = 24;
	rightLED[3] = 24;
	rightLED[4] = 24;
	rightLED[5] = 24;
}

delay_ms(100);	/* Allow all scanners to finish initializing before lighting any LEDs.
				 * This is necessary because the clock speeds on each chip only have a 10% tolerance, and initial
				 * testing showed that the tests to configure far_left and far_right were causing
				 * "faster" chips to get triggered by "slower" chips.
				 */
for (;;)  // main loop
{
	loopcount++;

	if (loopcount > delaytime)
	{
		loopcount = 0;
		
	if ((PINB & 16) == 0) // Check for button press
	{ 
		if ((initloopcount >= 4) & (far_left == 1) & (already_running == 0))
		{
			runitout = 1;
			already_running = 1;
			debounce = 0;  // Start running the program, but only on the left-most unit.
		}
		
/*	This is the section from the original program to let the button change the speeds and brightness.
		debounce2++;
		
		if (debounce2 > 100) 
		{   debounce2 = 0;
			debounce = 0;
			
		 if (BrightMode == 0) 
			BrightMode = 1;
		 else
			BrightMode = 0;
			
		}
		
		if (debounce)
		{ 
			speedLevel++;
			  
			if ((speedLevel == 2) || (speedLevel == 3)) { 
				delaytime = 0;
			} 
			else 
			{   speedLevel = 1; 
				delaytime = 1;
			}
			 
		debounce = 0;
		}
	}	
	else{ 
	debounce = 1;
	debounce2 = 1;
	}
*/	

	}
	
	if ((PINA & 1) == 0)  // Check to see if display from the left has triggered to start
	{
		direction = 0;
		runitout = 1;
	}
	
	if ((PIND & 1) == 0)  // Check to see if display from the right has triggered to start
	{
		direction = 1;
		runitout = 1;
	}

	    
	if (runitout)
	{
		position++;
		
		if (speedLevel == 3)
			position++;
		
		if ((softbounce == 1) & (((direction == 0) & (far_right == 1)) || ((direction == 1) & (far_left == 1))) & (position >= 224))
		{ // this allows us to "soft-bounce" off the ends, like in the original program, without running off the edge
			position = 15;
			if (direction == 0)
				direction = 1;
			else
				direction = 0;
		}
		
		if (position >= 240)	// To allow for runoff at the ends; was '== 128'
		{
			position = 0;
		 
			if (direction == 0) {  
				direction = 1;  // we've reached the end, so go back in the other direction
				if (far_right == 0) //  If this isn't the end of the chain, we want to stop, and wait for a signal to go again
					runitout = 0;
			}
			else {
				direction = 0;
				if (far_left == 0)  //  If this isn't the end of the chain, we want to stop, and wait for a signal to go again
					runitout = 0;
			}
		}
		 		
	if (direction == 0)  // Moving to right, as viewed from front.
	{   
		ILED = (15+position) >> 4; 
		RLED = (15+position) - (ILED << 4);
		MLED = 15 - RLED; 		
	}
    else 
	{   
		ILED = (127 - position) >> 4;  
		MLED = (127 - position)  - (ILED << 4);
		RLED =  15 - MLED;	
	}
	
	if ((ILED == 10) & (direction == 0) & (far_right == 0))
		d_mod = 2; // If we're heading to the right, and we're not at the end of the chain, we want to trigger D1 to start the next scanner
		
	j = 0;
	while (j < 9) {
		LEDs[j] = 0; 
		j++;
	}  
		 
	j = 0;
	if ((softbounce == 1) & (((direction == 0) & (far_right == 1)) || ((direction == 1) & (far_left == 1)))) { 
		while (j < 5) {
			
			if (direction == 0) 
			   m = ILED - (j + 1);
			   //m = ILED + (2 - j);	// e.g., eyeLoc[0] = ILED + 2; 
			else
			   m = ILED + (j + 1);
			   //m = ILED + (j - 2);  // e.g., eyeLoc[0] = ILED - 2;
			
			if ((direction == 0) & (m > 8))
				m = 8;
			
			if ((direction == 1) & (m < 0))
				m = 0;
			
			eyeLoc[j] = m;
			
			j++;
		}
	}
	else {
		while (j < 5)
		{		
			if (direction == 0) 
				m = ILED - (j + 1);	// e.g., eyeLoc[0] = ILED - 1; 
			else
				m = ILED + (j + 1); // e.g., eyeLoc[0] = ILED + 1;
			
			if ((m == -1) & (direction == 1) & (far_left == 0))
				a_mod = 2; // If we're heading to the left, and we're not at the end of the chain, we want to trigger A1 to start the next scanner
		
			if (m > 8)
				m = -1;  // If eye position is past the end of the board, don't light it; set to -1
						
			if (m < 0)
				m = -1;  // If eye position is past the end of the board, don't light it; set to -1
			
			eyeLoc[j] = m;
			
			j++;
		} 
	}
		  
	j = 0;		// For each of the eye parts...
	while (j < 4) 
	{		
		if (eyeLoc[j] >= 0)
			LEDs[eyeLoc[j]]   += LEDBright[j]*RLED;	
		if (eyeLoc[j+1] >= 0)
			LEDs[eyeLoc[j+1]] += LEDBright[j]*MLED;
		j++;
	}  
	
	 LED0 = LEDs[0];
     LED1 = LEDs[1];
	 LED2 = LEDs[2];
	 LED3 = LEDs[3];
	 LED4 = LEDs[4];
	 LED5 = LEDs[5];
	 LED6 = LEDs[6];
	 LED7 = LEDs[7];
	 LED8 = LEDs[8];  
	}
	else if (initloopcount < 4 )
	{
		k = 0;
		while (k < 6) {
			PORTD = leftLED[k];
			PORTB = rightLED[k];
			delay_ms(1);
		
			PORTD = d_base;
			PORTB = 16;
			delay_ms(29);

			k++;
		}
		delay_ms(100);
		initloopcount++;
	}
	}
	if (runitout) {
	if (BrightMode == 0)
	{		//Multiplexing routine: Each LED is on (1/9) of the time. 
			//  -> Use much less power.
		j = 0;
		PORTA = a_base - a_mod; // we set a_mod to correspond to A1's bit in PORTA; this makes it easier to change the pin configs later
		while (j < 60)		// Truncate brightness at a max value (60) in the interest of speed.
		{
		  
	if (LED0 > j) 
		PORTD = 7 - d_mod;  // we set d_mod to correspond to D1's bit in PORTD; this makes it easier to change the trigger pin configs later
	else	
		PORTD = d_base - d_mod;  
			
	if (LED1 > j) 
		PORTD = 11 - d_mod;	
	else	
		PORTD = d_base - d_mod;	 
			
	if (LED2 > j) 
		PORTD = 19 - d_mod; 
	else	
		PORTD = d_base - d_mod; 
			
	if (LED3 > j) 
		PORTD = 35 - d_mod;	
	else	
		PORTD = d_base - d_mod;
			
	if (LED4 > j) 
		PORTD = 67 - d_mod;
	else	
		PORTD = d_base - d_mod;
		  
	if (LED5 > j) {
		PORTB = 17;	 
		PORTD = d_base - d_mod;}
	else	{
		PORTB = 16;
		PORTD = d_base - d_mod;}
			
	if (LED6 > j) 
		PORTB = 18;	
	else	
		PORTB = 16;
			
	if (LED7 > j) 
		PORTB = 20;	
	else	
		PORTB = 16;
			
	if (LED8 > j) 
		PORTB = 24;	
	else	
		PORTB = 16; 
		 
		j++;
//		if (speedLevel == 3)
//			j++;
		 PORTB = 16; 
	} 
	 
	d_mod = 0;
	a_mod = 0;
	} 
	 else 
	 {		// Full power routine
	 
	  PORTA = a_base - a_mod;

	  j = 0;
	  while (j < 70)
	  {
	  
	  
	  pt = d_base - d_mod;	
	  if (LED0 > j) 
	  pt |= 4; 
	  if (LED1 > j) 
	  pt |= 8;	
	  if (LED2 > j) 
	  pt |= 16; 
	  if (LED3 > j) 
	  pt |= 32;		 
	  if (LED4 > j) 
	  pt |= 64;
	  
	  PORTD = pt;
	  shortdelay(); 
	  pt = 16;	
	  if (LED5 > j) 
	  pt |= 1;	
	  if (LED6 > j) 
	  pt |= 2;	
	  if (LED7 > j) 
	  pt |= 4;	
	  if (LED8 > j) 
	  pt |= 8;			
	  
	  PORTB = pt;
	  shortdelay(); 
		  
	  j++;
//	  if (speedLevel == 3)
//	  j++;
	  }
	
	 d_mod = 0; // we want to stop triggering D1, so set the modifier of PORTD back to 0
	 a_mod = 0; // we want to stop triggering A1, so set the modifier of PORTA back to 0
	 
	 }
	}
	 
//Multiplexing routine: Each LED is on (1/9) of the time. 
//  -> Uses much less power.
   PORTA = a_base - a_mod;
	}	//End main loop
	return 0;
}
