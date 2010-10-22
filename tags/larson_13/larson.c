/*
larson.c
The Larson Scanner 

Written by Windell Oskay, http://www.evilmadscientist.com/

 Copyright 2009 Windell H. Oskay
 Distributed under the terms of the GNU General Public License, please see below.
 
 

 An avr-gcc program for the Atmel ATTiny2313  
 
 Version 1.3   Last Modified:  2/8/2009. 
 Written for Evil Mad Science Larson Scanner Kit, based on the "ix" circuit board. 
 
 Improvements in v 1.3:
 * EEPROM is used to *correctly* remember last speed & brightness mode.
 
 Improvements in v 1.2:
 * Skinny "eye" mode.  Hold button at turn-on to try this mode.  To make it default,
 solder jumper "Opt 1."  (If skinny mode is default, holding button will disable it temporarily.)
 * EEPROM is used to remember last speed & brightness mode.
 
 
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

#include <avr/eeprom.h> 

#define shortdelay(); 			asm("nop\n\t" \
"nop\n\t");


uint16_t eepromWord __attribute__((section(".eeprom")));

int main (void)
{
uint8_t LEDs[9]; // Storage for current LED values
	
int8_t eyeLoc[5]; // List of which LED has each role, leading edge through tail.

uint8_t LEDBright[4] = {1U,4U,2U,1U};   // Relative brightness of scanning eye positions
	
int8_t j, m;
	
uint8_t position, loopcount, direction;
uint8_t ILED, RLED, MLED;

uint8_t delaytime;
	
	uint8_t skinnyEye = 0;
uint8_t  pt, debounce, speedLevel;
	uint8_t 	UpdateConfig;
	uint8_t BrightMode;
	uint8_t debounce2, modeswitched;
	
	uint8_t CycleCountLow;  
	uint8_t LED0, LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8;
	
//Initialization routine: Clear watchdog timer-- this can prevent several things from going wrong.		
MCUSR &= 0xF7;		//Clear WDRF Flag
WDTCSR	= 0x18;		//Set stupid bits so we can clear timer...
WDTCSR	= 0x00;

//Data direction register: DDR's
//Port A: 0, 1 are inputs.	
//Port B: 0-3 are outputs, B4 is an input.	
//Port D: 1-6 are outputs, D0 is an input.
	
	DDRA = 0U;
	DDRB = 15U;	
	DDRD = 126U;
	
	PORTA = 3;	// Pull-up resistors enabled, PA0, PA1
	PORTB = 16;	// Pull-up resistor enabled, PA
	PORTD = 0;
	
/* Visualize outputs:
 
 L to R:
 
 D2 D3 D4 D5 D6 B0 B1 B2 B3	
  
*/
	
//multiplexed = LowPower;	
 
	debounce = 0; 
	debounce2 = 0;
	loopcount = 254; 
	delaytime = 0;
	
	direction = 0;
	position = 0;
	speedLevel = 2;  // Range: 1, 2, 3
	BrightMode = 0;
	CycleCountLow = 0;
	UpdateConfig = 0;
	modeswitched = 0;
	
	
	if ((PINA & 2) == 0)		// Check if Jumper 1, at location PA1 is shorted
	{  
		// Optional place to do something.  :)
	}
	
	
	
	if ((PINA & 1) == 0)		// Check if Jumper 2, at location PA0 is shorted
	{    
		skinnyEye = 1; 
	}	
	
	
	if ((PINB & 16) == 0)		// Check if button pressed pressed down at turn-on
	{		//Toggle Skinnymode
		if (skinnyEye)
			skinnyEye = 0; 
		else 
			skinnyEye = 1;
	}
	
	
	
	if (skinnyEye){
		LEDBright[0] = 0;
		LEDBright[1] = 4;
		LEDBright[2] = 1;
		LEDBright[3] = 0;  
	}
	
	
	//Check EEPROM values:
	
	pt = (uint8_t) (eeprom_read_word(&eepromWord)) ;
	speedLevel = pt >> 4;
	BrightMode = pt & 1;
	
	if (pt == 0xFF)
	{
		BrightMode = 0;
	}
	
	
	if (speedLevel > 3)
		speedLevel = 1; 
	
	if ((speedLevel == 2) || (speedLevel == 3)) { 
		delaytime = 0;
	} 
	else 
	{   speedLevel = 1; 
		delaytime = 1;
	}	
	
	 
	
for (;;)  // main loop
{
	loopcount++;
	
	if (loopcount > delaytime)
	{
		loopcount = 0;
		
		CycleCountLow++;
		if (CycleCountLow > 250)  
			CycleCountLow = 0;
		
		
		if (UpdateConfig){		// Need to save configuration byte to EEPROM 
			if (CycleCountLow > 100) // Avoid burning EEPROM in event of flaky power connection resets
			{
				
				UpdateConfig = 0;
				pt = (speedLevel << 4) | (BrightMode & 1); 
				eeprom_write_word(&eepromWord, (uint8_t) pt);	
				// Note: this function causes a momentary brightness glitch while it writes the EEPROM.
				// We separate out this section to minimize the effect. 
			}
			
		}	
		
		
	if ((PINB & 16) == 0)		// Check for button press
	{
		debounce2++;
		
		if (debounce2 > 100)  
		{   
			if (modeswitched == 0)
			{  
			debounce2 = 0;
			UpdateConfig = 1;
			
		 if (BrightMode == 0) 
			BrightMode = 1;
		 else
			BrightMode = 0;
			
			
			modeswitched = 1;
			}
		}
		else {
			debounce = 1;		// Flag that the button WAS pressed.
			debounce2++;	
		}
 
	}	
	else{ 
		
		debounce2 = 0;
		modeswitched = 0;
	
		if (debounce)
		{ debounce = 0;
			speedLevel++;
			UpdateConfig = 1;
			
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
	    
		position++;
		
		if (speedLevel == 3)
			position++;
		
	 if (position >= 128)	//was  == 128
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
			   m = ILED + (2 - j);	// e.g., eyeLoc[0] = ILED + 2; 
			else
			   m = ILED + (j - 2);  // e.g., eyeLoc[0] = ILED - 2;
			
			if (m > 8)
				m -= (2 * (m - 8));
			
			if (m < 0)
				m *= -1;
			
			eyeLoc[j] = m;
			
			j++;
		} 
		  
		j = 0;		// For each of the eye parts...
		while (j < 4) {
			
			LEDs[eyeLoc[j]]   += LEDBright[j]*RLED;			
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
