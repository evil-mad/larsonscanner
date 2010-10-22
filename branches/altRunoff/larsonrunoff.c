/*
larsonrunoff.c
The Larson Scanner -- Alternative version to allow scanner to run off the edge of the board.
 
 It simulates one LED at brightness 4, followed by one LED of brightness 1, that moves across
 the nine pixels, disappearing off either end of the board, before returning to scan in the other direction.
 
 There is no longer any overlap of these "LEDs" at either end, but up to three LEDs may be lit at a time as
 the head fades in and the tail fades out.
 
 Also, some of the input and output values and pull-up resistors have been changed from the original program 
 in anticipation of future extensibility.

Original written by Windell Oskay, http://www.evilmadscientist.com/
New alternative version written by John Breen III
 
 Copyright 2009 Windell H. Oskay, 2010 John J. Breen III
 Distributed under the terms of the GNU General Public License, please see below.
 
 

 An avr-gcc program for the Atmel ATTiny2313  
 
 Based on Version 1.1_alt1, written by Windell Oskay
 
 Version 1.0   Last Modified:  17-Aug-2010. 
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
	
int8_t eyeLoc[5]; // List of which LED has each role, leading edge through tail.

uint8_t LEDBright[4] = {0U,4U,1U,0U};   // Relative brightness of scanning eye positions, head through tail
	
int8_t j, m;
	
uint8_t position, loopcount, direction;
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
	
	PORTA = 1;	// Pull-up resistor enabled, PA0 
	PORTB = 16;	// Pull-up resistor enabled, PB4
	PORTD = 1;  // Pull-up resistor enabled, PD0
	
/* Visualize outputs:
 
 L to R:
 
 D2 D3 D4 D5 D6 B0 B1 B2 B3	
  
*/
	
//multiplexed = LowPower;	
 
	debounce = 1; 
	debounce2 = 1;
	loopcount = 254; 
	delaytime = 0;
	
	direction = 0;
	position = 0;
	speedLevel = 2;  // Range: 1, 2, 3
	BrightMode = 0;
	
for (;;)  // main loop
{
	loopcount++;
	
	if (loopcount > delaytime)
	{
		loopcount = 0;
		
	if ((PINB & 16) == 0)		// Check for button press
	{
		debounce2++;
		
		if (debounce2 > 100) 
		{   debounce2 = 0;
			
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
	    
		position++;
		
		if (speedLevel == 3)
			position++;
		
	 if (position >= 208)	// To allow for runoff at the ends
	 {
		 position = 0;
		 
	  if (direction == 0)
		  direction = 1;
	  else
		  direction = 0; 
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
		 
		j = 0;
		while (j < 9) {
			LEDs[j] = 0; 
			j++;
		}  
		 
		j = 0;
		while (j < 5) {
			
			if (direction == 0) 
			   m = ILED - (j + 1);	// e.g., eyeLoc[0] = ILED - 1; 
			else
			   m = ILED + (j + 1);  // e.g., eyeLoc[0] = ILED + 1;
			
			if (m > 8)
				m = -1;  // If eye position is past the end of the board, don't light it; set to -1
			
			if (m < 0)
				m = -1;  // If eye position is past the end of the board, don't light it; set to -1
			
			eyeLoc[j] = m;
			
			j++;
		} 
		  
		j = 0;		// For each of the eye parts...
		while (j < 4) {
			
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
	 
	if (BrightMode == 0)
	{		//Multiplexing routine: Each LED is on (1/9) of the time. 
			//  -> Use much less power.
		j = 0;
		while (j < 60)		// Truncate brightness at a max value (60) in the interest of speed.
		{
		  
	if (LED0 > j) 
		PORTD = 4; 
			else	
			PORTD = 0;  
			
	if (LED1 > j) 
		PORTD = 8;	
	else	
		PORTD = 0;	 
			
	if (LED2 > j) 
		PORTD = 16; 
	else	
		PORTD = 0; 
			
	if (LED3 > j) 
		PORTD = 32;	
	else	
		PORTD = 0;
			
	if (LED4 > j) 
		PORTD = 64;
	else	
		PORTD = 0;
		  
	if (LED5 > j) {
		PORTB = 17;	 
		PORTD = 0;}
			else	{
		PORTB = 16;
	PORTD = 0;		}
			
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
	 
	} 
	 else 
	 {		// Full power routine
		 
	  j = 0;
	  while (j < 70)
	  {
	  
	  pt = 0;	
	  if (LED0 > j) 
	  pt = 4; 
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
		   
	 }
	
	 
//Multiplexing routine: Each LED is on (1/9) of the time. 
//  -> Uses much less power.
   
	}	//End main loop
	return 0;
}
