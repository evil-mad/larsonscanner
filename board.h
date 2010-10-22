/**
 * @file    board.h
 * @brief   Board specific include file.
 * @details Evil Mad Science Larson Scanner Kit, based on the "ix" circuit 
 *          board with Atmel ATTiny2313.
 *
 *          More information about this project is at:
 *          http://www.evilmadscientist.com/article.php/larsonkit
 *          http://code.google.com/p/larsonscanner
 * 
 * @date    October, 2010
 * @author  Jacob Madden
 * Distributed under the terms of the GNU General Public License
 */
// PINA defines
#define JUMPER1_BIT         0x02
#define JUMPER2_BIT         0x01
// PINB defines
#define BUTTON_BIT          0x10

#define BUTTON_IS_PRESSED   ((PINB & BUTTON_BIT) == 0)
#define JUMPER1_SHORTED     ((PINA & JUMPER1_BIT) == 0)
#define JUMPER2_SHORTED     ((PINA & JUMPER2_BIT) == 0)

// LED processor connections left to right (LED D0-D8)  
// Port pins: D2 D3 D4 D5 D6 B0 B1 B2 B3

// PortD
#define LOWER_LED_PORT      PORTD
#define NUM_LOWER_LEDS      5
#define LED0_BIT            0x04
#define LED1_BIT            0x08
#define LED2_BIT            0x10
#define LED3_BIT            0x20
#define LED4_BIT            0x40

// PortB
#define UPPER_LED_PORT      PORTB
#define NUM_UPPER_LEDS      4
#define LED5_BIT            0x01
#define LED6_BIT            0x02
#define LED7_BIT            0x04
#define LED8_BIT            0x08
