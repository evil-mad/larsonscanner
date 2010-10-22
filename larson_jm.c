/**
 * @file    larson_jm.c
 * @brief   The Larson Scanner
 * @details Restructured version of larson.c Version 1.3 by Windell Oskay.
 *
 * An avr-gcc program for the Atmel ATTiny2313  
 * 
 * Written for Evil Mad Science Larson Scanner Kit, based on the "ix" circuit 
 * board. 
 * 
 * More information about this project is at 
 * http://www.evilmadscientist.com/article.php/larsonkit
 * http://code.google.com/p/larsonscanner
 * 
 * -------------------------------------------------
 * USAGE: How to compile and install
 * 
 * A makefile is provided to compile and install this program using AVR-GCC 
 * and avrdude.
 * 
 * To use it, follow these steps:
 * 1. Update the header of the makefile as needed to reflect the type of 
 * AVR programmer that you use.
 * 2. Open a terminal window and move into the directory with this file and 
 * the makefile.  
 * 3. At the terminal enter
 * make clean   <return>
 * make all     <return>
 * make install <return>
 * 4. Make sure that avrdude does not report any errors.  If all goes well, 
 * the last few lines output by avrdude
 * should look something like this:
 * 
 * avrdude: verifying ...
 * avrdude: XXXX bytes of flash verified
 * 
 * avrdude: safemode: lfuse reads as 62
 * avrdude: safemode: hfuse reads as DF
 * avrdude: safemode: efuse reads as FF
 * avrdude: safemode: Fuses OK
 * 
 * avrdude done.  Thank you.
 * If you a different programming environment, make sure that you copy 
 * over the fuse settings from the makefile.
 * -------------------------------------------------
 * 
 * @date    October, 2010
 * @version 1.0
 * @author  Jacob Madden
 * @author  Windell Oskay, http://www.evilmadscientist.com/
 * Copyright (C) 2009 Windell H. Oskay
 * Distributed under the terms of the GNU General Public License
 *
 * https://larsonscanner.googlecode.com/svn/branches/larson_jm
 */
#include <stdbool.h>
#include <avr/io.h> 
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "board.h"

#define shortdelay(); 	asm("nop\n\tnop\n\t");

// Limits for the position
#define POSITION_INITIAL        1
#define POSITION_LIMIT          9

// Direction that the LEDs are strobing
#define DIR_MOVING_RIGHT        1 // from front view
#define DIR_MOVING_LEFT         -1

// Brightness options
#define MAX_BRIGHTNESS          100
#define HIGH_BRIGHTNESS         85
#define MEDIUM_HIGH_BRIGHTNESS  70
#define MEDIUM_BRIGHTNESS       50
#define MEDIUM_LOW_BRIGHTNESS   30
#define LOW_BRIGHTNESS          15
#define NO_BRIGHTNESS           0

// Options for how the LEDs are updated
#define BRIGHT_MODE_POWER_SAVE  0
#define BRIGHT_MODE_FULL_POWER  1

#define NUM_TOT_LEDS            9
#define MAX_LED_INDEX           (NUM_TOT_LEDS-1)

// Speed settings
#define MIN_SPEED               1
#define MAX_SPEED               3

// The number of differnt demo display modes
#define NUM_DISPLAY_MODES       6

// A way to adjust the timing, higher equals slower
#define DEFAULT_SLOWNESS        5

// The bin width when displaying intensity values
#define INTENSITY_DELTA         10

// Storage for application settings
typedef struct _LarsonSettings
{
  uint8_t speed_level;
  uint8_t led_relative_brightness[4]; // for orig version
  uint8_t slowness;
  uint8_t display_mode;
  bool skinny_eye_enabled; // for orig version
} LarsonSettings;

// Storage for application data
typedef struct _LarsonRuntimeData
{
  int8_t direction;
  uint8_t position;
  uint8_t led_brightness[NUM_TOT_LEDS];
} LarsonRuntimeData;

// Static variables
static LarsonRuntimeData _data;
static LarsonSettings _settings;
static bool _eeprom_needs_update = false;

uint16_t eepromWord __attribute__((section(".eeprom")));

#define TCNT0_INIT    0x52
#define OCR0_INIT     0xE4
/**
 * Initializes Timer0.
 */
void timer0_init(void)
{
  TCCR0B = 0x00; // no clocking source, timer stopped
  TCNT0 = TCNT0_INIT; // set count
  OCR0A = OCR0_INIT; // set count
  OCR0B = OCR0_INIT; // set count
  // set timer control registers, normal operation
  TCCR0A = 0x00;
  TCCR0B = 0x01; // timer source = clk_io, timer started
  TIMSK = (1<<TOIE0); // enable timer interrupt
}

// store bits in array for easy loop updates
static const uint8_t lower_led_bits[] = {LED0_BIT, LED1_BIT, LED2_BIT, 
                                  LED3_BIT, LED4_BIT};
static const uint8_t upper_led_bits[] = {LED5_BIT, LED6_BIT, LED7_BIT, LED8_BIT};

/**
 * Timer0 overflow interrupt for setting the pulse width seen by each LED.
 * @details Each LED is on for led_brightness/MAX_BRIGHTNESS percentage
 *          of the time.
 */
ISR(TIMER0_OVF_vect)
{
  static uint8_t cnts = 0;
  uint8_t i;
  uint8_t *led_ptr = &_data.led_brightness[0];
  TCNT0 = TCNT0_INIT; // reload counter value
  
  for(i=0; i < NUM_LOWER_LEDS; ++i)
  {
    if(*led_ptr++ > cnts)
    {
      LOWER_LED_PORT |= lower_led_bits[i];
    }
    else
    {
      LOWER_LED_PORT &= ~lower_led_bits[i];
    }
  }
  for(i=0; i < NUM_UPPER_LEDS; ++i)
  {
    if(*led_ptr++ > cnts)
    {
      UPPER_LED_PORT |= upper_led_bits[i];
    }
    else
    {
      UPPER_LED_PORT &= ~upper_led_bits[i];
    }
  }
  cnts++;
  if(cnts > MAX_BRIGHTNESS)
  {
    cnts = 0;
  }
}

/**
 * Initializes the board.
 */
void init_board(void)
{
  // Initialization routine: Clear watchdog timer
  // -- this can prevent several things from going wrong.		
  MCUSR  &= 0xF7; // Clear WDRF Flag
  WDTCSR = 0x18;  // Set stupid bits so we can clear timer...
  WDTCSR = 0x00;
  
  // Data direction register: DDR's
  // Port A: 0, 1 are inputs.	
  // Port B: 0-3 are outputs, B4 is an input.	
  // Port D: 1-6 are outputs, D0 is an input.
  DDRA = 0x00;
  DDRB = 0x0F;	
  DDRD = 0x7E;
  
  PORTA = 0x03;	// Pull-up resistors enabled, PA0, PA1
  PORTB = 0x10;	// Pull-up resistor enabled, PA
  PORTD = 0x00;
}

/**
 * Updates the speed settings.
 */
void update_speed_setting(void)
{
  if (_settings.speed_level > MAX_SPEED)
  {
    _settings.speed_level = MIN_SPEED; 
  }
}

/**
 * Initializes the app settings.
 */
void settings_init(void)
{
  uint8_t pt;
  _settings.speed_level = 2;
  _settings.skinny_eye_enabled = false;
  _settings.slowness = DEFAULT_SLOWNESS;
  _settings.display_mode = 0;
  // default relative brightness
  _settings.led_relative_brightness[0] = 1;
  _settings.led_relative_brightness[1] = 4;
  _settings.led_relative_brightness[2] = 2;
  _settings.led_relative_brightness[3] = 1;
  
  if (JUMPER1_SHORTED) // Check if Jumper 1, at location PA1 is shorted
  {  
    // Optional place to do something.  :)
  }
  
  if (JUMPER2_SHORTED) // Check if Jumper 2, at location PA0 is shorted
  {    
    _settings.skinny_eye_enabled = true; 
  }	
  
  if (BUTTON_IS_PRESSED) // Check if button pressed pressed down at turn-on
  { // Toggle Skinnymode
    _settings.skinny_eye_enabled = !_settings.skinny_eye_enabled;
  }
  
  // Check EEPROM values:
  pt = (uint8_t) (eeprom_read_word(&eepromWord)) ;
  _settings.speed_level = pt >> 4;
  
  update_speed_setting();
  
  if (_settings.skinny_eye_enabled)
  {
    _settings.led_relative_brightness[0] = 0;
    _settings.led_relative_brightness[1] = 4;
    _settings.led_relative_brightness[2] = 1;
    _settings.led_relative_brightness[3] = 0;
  }
}

// macros specific to the older version
#define POSITION_LIMIT_OLD_VER  127
#define NUM_ON_LEDS             5
/**
 * The original Larson scanner update strategy.
 */
void update_led_brightness_larson_orig(void)
{
  uint8_t on_led_indices[NUM_ON_LEDS]; // List of which LED has each role, 
                                       // leading edge through tail
  static uint8_t position_o = 1;
  static int8_t dir_o = DIR_MOVING_RIGHT;
  uint8_t center_led_index;
  uint8_t led_brightness_r;
  uint8_t led_brightness_m;
  int8_t j;
  uint8_t new_brightness[NUM_TOT_LEDS];

  if (dir_o == DIR_MOVING_RIGHT)
  {
    // grab top nibble to select center LED index, increasing with position
    center_led_index = (0x0F + position_o) >> 4;
    // grab bottom nibble to select brightness value (0x0-0xF)
    led_brightness_r = (0x0F + position_o) - (center_led_index << 4);
    // inverse brightness value (0x0-0xF)
    led_brightness_m = 0xF - led_brightness_r; 
  }
  else 
  { // moving left
    // grab top nibble to select center LED index, decreasing with position
    center_led_index = (POSITION_LIMIT_OLD_VER - position_o) >> 4;
    // grab bottom nibble to select brightness value (0x0-0xF)
    led_brightness_m = (POSITION_LIMIT_OLD_VER - position_o)  - 
      (center_led_index << 4);
    // inverse brightness value (0x0-0xF)
    led_brightness_r =  0xF - led_brightness_m;	
  }
  
  // clear previous brightness values
  for(j=0; j < NUM_TOT_LEDS; ++j)
  {
    new_brightness[j] = 0;
  }
  
  // set indices centered around center LED   
  for(j=0; j < NUM_ON_LEDS; ++j)
  {
    int8_t new_index;
    if (dir_o == DIR_MOVING_RIGHT)
    {
      new_index = center_led_index + (2 - j); // e.g., eyeLoc[0] = ILED + 2; 
    }
    else
    {
      new_index = center_led_index + (j - 2); // e.g., eyeLoc[0] = ILED - 2;
    }
    
    // roll back index the other way
    if (new_index > MAX_LED_INDEX)
    {
      new_index = (2*MAX_LED_INDEX) - new_index;
    }
    
    if (new_index < 0)
    {
      new_index *= -1;
    }
    
    on_led_indices[j] = (uint8_t)new_index;
  } 
  
  // Update brightness values for each ON led
  for(j=0; j < NUM_ON_LEDS-1; ++j) 
  {
    new_brightness[on_led_indices[j]]   += 
      _settings.led_relative_brightness[j]*led_brightness_r;			
    new_brightness[on_led_indices[j+1]] += 
      _settings.led_relative_brightness[j]*led_brightness_m;			
  }
  
  for(j=0; j < NUM_TOT_LEDS; ++j)
  {
    _data.led_brightness[j] = new_brightness[j];
  }
  
  // position/speed now used differently, need to do manually for orig version
  position_o += 25;
  _settings.slowness = 0;
  if(_settings.speed_level == 3)
  {
    position_o += 10;
  }
  else if(_settings.speed_level == MIN_SPEED)
  {
  }
  else
  {
    position_o += 5;
  }
  if(position_o > POSITION_LIMIT_OLD_VER)
  {
    position_o = 1;
    dir_o = -dir_o;
  }
}

/**
 * Larson scanner display using a new update strategy. Only three LEDs 
 * at a time.
 */
 
void update_led_brightness_larson2(void)
{
  uint8_t i;
  uint8_t center_led_index = _data.position-1;
  uint8_t new_brightness[NUM_TOT_LEDS];
  
  for(i=0; i < NUM_TOT_LEDS; ++i)
  {
    new_brightness[i] = 0;
  }
  new_brightness[center_led_index] = MEDIUM_HIGH_BRIGHTNESS;
  if(center_led_index > 0)
  {
    new_brightness[center_led_index-1] = LOW_BRIGHTNESS;
  }
  if(center_led_index < MAX_LED_INDEX)
  {
    new_brightness[center_led_index+1] = LOW_BRIGHTNESS;
  }
  for(i=0; i < NUM_TOT_LEDS; ++i)
  {
    _data.led_brightness[i] = new_brightness[i];
  }
}

/**
 * Displays intensity growing out from the center.
 */
#define MIDDLE_LED_INDEX    (NUM_TOT_LEDS/2)
void update_led_brightness_intensity_middle(uint8_t intensity)
{
  uint8_t i;
  for(i=0; i < (NUM_TOT_LEDS-MIDDLE_LED_INDEX); ++i)
  {
    if(intensity > INTENSITY_DELTA)
    {
      intensity -= INTENSITY_DELTA;
      _data.led_brightness[MIDDLE_LED_INDEX+i] = MEDIUM_BRIGHTNESS;
      _data.led_brightness[MIDDLE_LED_INDEX-i] = MEDIUM_BRIGHTNESS;
    }
    else
    {
      _data.led_brightness[MIDDLE_LED_INDEX+i] = 0;
	  _data.led_brightness[MIDDLE_LED_INDEX-i] = 0;
    }
  }
}

/**
 * Uses the LED array as a bar indicator. Could be used for displaying
 * the loudness of music or some other intensity value.
 * @param[in] intensity The intensity to display on the LEDs
 */
void update_led_brightness_intensity_left(uint8_t intensity)
{
  uint8_t i;
  for(i=0; i < NUM_TOT_LEDS; ++i)
  {
    if(intensity > INTENSITY_DELTA)
    {
      intensity -= INTENSITY_DELTA;
      _data.led_brightness[i] = MEDIUM_BRIGHTNESS;
    }
    else
    {
      _data.led_brightness[i] = 0;
    }
  }
}

/**
 * Visualize the brightness settings of the LEDs.
 */
void update_led_brightness_glow_left(void)
{
  uint8_t i;
  static uint8_t curr_led = 0;
  
  if(_data.position < 6) // update slowly
  {
    _data.led_brightness[curr_led]++;
    if(_data.led_brightness[curr_led] >= MEDIUM_BRIGHTNESS)
    {
      curr_led++;
      if(curr_led > MAX_LED_INDEX)
      {
        // start over
        curr_led = 0;
        _data.led_brightness[0] = 0;
      }
    }
    for(i=curr_led+1; i < NUM_TOT_LEDS; ++i)
    {
      _data.led_brightness[i] = 0;
    }
  }
}

#define KNIGHT_RIDER_BRIGHTNESS_DELTA   5
/**
 * Displays something similar to KITT from Knight Rider.
 */
void update_led_brightness_knight_rider(void)
{
  uint8_t i;
  uint8_t center_led_index = _data.position-1;
  int8_t low_index = center_led_index - 1;
  int8_t high_index = center_led_index + 1;
  int8_t low_brightness = LOW_BRIGHTNESS;
  int8_t high_brightness = LOW_BRIGHTNESS;
  uint8_t new_brightness[NUM_TOT_LEDS];
  
  for(i=0; i < NUM_TOT_LEDS; ++i)
  {
    new_brightness[i] = 0;
  }
  new_brightness[center_led_index] = MEDIUM_HIGH_BRIGHTNESS;

  while(low_index >= 0)
  {
    if(low_brightness > 0)
    {
      new_brightness[low_index] = low_brightness;
      low_brightness -= KNIGHT_RIDER_BRIGHTNESS_DELTA;
    }
    else
    {
      new_brightness[low_index] = 0;
    }
    low_index--;
    if(_data.direction == DIR_MOVING_LEFT)
    {
      // only do one LED ahead
      break;
    }
  }
  while(high_index <= MAX_LED_INDEX)
  {
    new_brightness[high_index] = high_brightness;
    if(high_brightness > 0)
    {
      new_brightness[high_index] = high_brightness;
      high_brightness -= KNIGHT_RIDER_BRIGHTNESS_DELTA;
    }
    else
    {
      new_brightness[high_index] = 0;
    }
    high_index++;
    if(_data.direction == DIR_MOVING_RIGHT)
    {
      // only do one LED ahead
      break;
    }
  }
  for(i=0; i < NUM_TOT_LEDS; ++i)
  {
    _data.led_brightness[i] = new_brightness[i];
  }
}

/**
 * Updates the LED position. For now scrolls between POSITION_INITIAL and 
 * POSITION_LIMIT.
 */
void update_position(void)
{
  _data.position += _data.direction;
  
  if (_data.position > POSITION_LIMIT)
  {
    // toggle direction
    _data.direction = -_data.direction;
    _data.position = POSITION_LIMIT - 1;
  }
  else if(_data.position < POSITION_INITIAL)
  {
    // toggle direction
    _data.direction = -_data.direction;
    _data.position = POSITION_INITIAL + 1;    
  }
}

/**
 * Toggles through the available display modes.
 */
void toggle_display_mode(void)
{
  _settings.display_mode++;
  if(_settings.display_mode >= NUM_DISPLAY_MODES)
  {
    _settings.display_mode = 0;
  }
}

/**
 * Updates the button press checking and handling.
 */
void update_button(void)
{
  static uint8_t debounce = 0;
  static uint8_t debounce2 = 0;
  static uint8_t modeswitched = 0;
  
  // Button debouncing and mode switching
  if (BUTTON_IS_PRESSED) // Check for button press
  {
    debounce2++;
    
    if (debounce2 > 100)  
    {   
      if (modeswitched == 0)
      {  
        debounce2 = 0;
        _eeprom_needs_update = true;
        
        modeswitched = 1;
      }
    }
    else 
    {
      debounce = 1;	// Flag that the button WAS pressed.
    }
  }	
  else
  {
    debounce2 = 0;
    modeswitched = 0;
    
    if (debounce)
    { 
      debounce = 0;
      _eeprom_needs_update = true;
      _settings.speed_level++;
      //update_speed_setting();
      // for now a button press switches the display mode
      toggle_display_mode();
    }
  }
}

/**
 * Updates the EEPROM configuration when necessary.
 */
void update_eeprom(void)
{
  static uint8_t cycle_count = 0;
  
  // Write configuration settings to EEPROM if necessary    
  cycle_count++;
  if (cycle_count > 250)
  {
    cycle_count = 0;
  }
  
  if (_eeprom_needs_update)
  {	// Need to save configuration byte to EEPROM 
    if (cycle_count > 100)
    { // Avoid burning EEPROM in event of flaky power connection resets
      uint8_t pt;
      _eeprom_needs_update = false;
      pt = (_settings.speed_level << 4); 
      eeprom_write_word(&eepromWord, (uint8_t) pt);	
      // Note: this function causes a momentary brightness glitch 
      // while it writes the EEPROM. We separate out this section 
      // to minimize the effect. 
    }
  }
}

#define DELAY_COUNTS    1
/**
 * Delays for an arbitrary amount of time.
 * @parm[in] delay_cycles   The number of cycles to delay
 */
void loop_delay(uint16_t delay_cycles) 
{
 uint8_t delay_counts = DELAY_COUNTS;
 volatile uint8_t i;

 while(delay_cycles > 0)
 {
   for (i=0; i < delay_counts; ++i)
   {
     // wait
   }
   delay_cycles--;
 }
}

/**
 * Main routine.
 * @return int 0 on success
 */
int main (void)
{
  init_board();
  settings_init();
  
  _data.direction = DIR_MOVING_RIGHT;
  _data.position = POSITION_INITIAL;
  timer0_init();
  sei(); // enable interrupts
  
  while(1)  // main loop
  {   
    //update_eeprom(); // don't use EEPROM right now
    update_button();
    update_position();
    
    switch(_settings.display_mode)
    {
    case 0:
      // original version of the Larson Scanner
      update_led_brightness_larson_orig();
      break;
    case 1:
      update_led_brightness_larson2();
      break;
    case 2:
      update_led_brightness_knight_rider();      
      break;
    case 3:
      update_led_brightness_intensity_middle(_data.position*11-5);
      break;
    case 4:
      update_led_brightness_intensity_left(_data.position*11-5);
      break;
    case 5:
      update_led_brightness_glow_left();
      break;
    default:
      update_led_brightness_larson_orig();
    }
    
    loop_delay(_settings.slowness);
  }
 
  return 0;
}
