#include "lcd.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "../gpio/gpio.h"
#include "../timeunits.h"

// RPi GPIO pin assignments
#define LCD_RS 25
#define LCD_E 24
#define LCD_D4 23
#define LCD_D5 10
#define LCD_D6 27
#define LCD_D7 22

/* Instructions */
// Clear display
#define LCD_CLEAR 0x01
// Return home
#define LCD_HOME 0x02

// Entry mode set
#define LCD_ENTRY_MODE				0x04
	#define LCD_EM_SHIFT_CURSOR		0
	#define LCD_EM_SHIFT_DISPLAY	0x01
	#define LCD_EM_DECREMENT		0
	#define LCD_EM_INCREMENT		0x02

// Display on/off control
#define LCD_DISPLAY_ONOFF			0x08
	#define LCD_DISPLAY_OFF			0
	#define LCD_DISPLAY_ON			0x04
	#define LCD_CURSOR_OFF			0
	#define LCD_CURSOR_ON			0x02
	#define LCD_CURSOR_NOBLINK		0
	#define LCD_CURSOR_BLINK		0x01

// Cursor or display shift
#define LCD_DISPLAY_CURSOR_SHIFT	0x10
	#define LCD_SHIFT_CURSOR		0
	#define LCD_SHIFT_DISPLAY		0x08
	#define LCD_SHIFT_LEFT			0
	#define LCD_SHIFT_RIGHT			0x04

// Function set
#define LCD_FUNCTION_SET			0x20
	#define LCD_FONT5x7				0
	#define LCD_FONT5x10			0x04
	#define LCD_ONE_LINE			0
	#define LCD_TWO_LINE			0x08
	#define LCD_4_BIT				0
	#define LCD_8_BIT				0x10

// Set CGRAM address
#define LCD_CGRAM_SET				0x40

// Set DDRAM address
#define LCD_DDRAM_SET				0x80


/**
 * Writes an 8-bit data to the LCD using D4-D7 pins
*/
static void write(uint8_t data);

/**
 * Writes 4 bits (a nibble) to the LCD using D4-D7 pins
*/
static void write_nibble(uint8_t nibble);

/**
 * Set all the pins used by the LCD as outputs
*/
static void init_pins(void)
{
	GPIO_set_out(LCD_RS);
	GPIO_set_out(LCD_E);
	GPIO_set_out(LCD_D4);
	GPIO_set_out(LCD_D5);
	GPIO_set_out(LCD_D6);
	GPIO_set_out(LCD_D7);

	// Wait for the input voltage to stabilize
	struct timespec delay = {
		.tv_sec = 0,
		.tv_nsec = MS_TO_NS(15),
	};
	nanosleep(&delay, NULL);

	GPIO_set_state(LCD_RS, 0);
	GPIO_set_state(LCD_E, 0);
}

static void set_4_bit_mode(void)
{
	struct timespec delay =
	{
		.tv_sec = 0,
		.tv_nsec = 0,
	};
	/*
	 * We need to change the HD44780 interface to use 4-bit data lines
	 * It is set to 8-bit mode by default so we have to execute the following
	 * loop first
	*/
	// Repeat the following 3 times
	for (uint8_t i = 0; i < 3; i++)
	{
		GPIO_set_state(LCD_E, 1);
		write_nibble(0x03);  // 8-bit mode
		GPIO_set_state(LCD_E, 0);
		delay.tv_nsec = MS_TO_NS(5);
		nanosleep(&delay, NULL);
	}

	// Set it to 4-bit mode
	GPIO_set_state(LCD_E, 1);
	write_nibble(0x02);  // 4-bit mode
	GPIO_set_state(LCD_E, 0);
}

static void set_up_display(void)
{
	struct timespec delay =
	{
		.tv_sec = 0,
		.tv_nsec = MS_TO_NS(1),
	};
	nanosleep(&delay, NULL);
	// Set the LCD to 4 bits, 2 lines, 5x7 font
	LCD_write_command(LCD_FUNCTION_SET | LCD_FONT5x7 | LCD_TWO_LINE | LCD_4_BIT);
	// Clear the display
	LCD_write_command(LCD_DISPLAY_ONOFF | LCD_DISPLAY_OFF);
	// Clear the DDRAM contents
	LCD_write_command(LCD_CLEAR);

	delay.tv_nsec = MS_TO_NS(2);
	nanosleep(&delay, NULL);

	// Address and cursor increment (when writing)
	LCD_write_command(LCD_ENTRY_MODE | LCD_EM_SHIFT_CURSOR | LCD_EM_INCREMENT);
	// Turn the LCD on, without the cursor and without blinking
	LCD_write_command(LCD_DISPLAY_ONOFF | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_CURSOR_NOBLINK);
}

void LCD_init(bool init_gpio)
{
	if (init_gpio)
		GPIO_init();

	init_pins();
	set_4_bit_mode();
	set_up_display();
}

void LCD_clear(void)
{
	LCD_write_command(LCD_CLEAR);
	struct timespec delay = {
		.tv_sec = 0,
		.tv_nsec = MS_TO_NS(2),
	};
	nanosleep(&delay, NULL);
}

void LCD_go_to(uint8_t x, uint8_t y)
{
	LCD_write_command(LCD_DDRAM_SET | (x + (0x40 * y)));
}

void LCD_write_text(char *text)
{
	while(*text)
		LCD_write_data(*text++);
}

void LCD_write_data(uint8_t data)
{
	GPIO_set_state(LCD_RS, 1);
	write(data);
}

void LCD_write_command(uint8_t command)
{
	GPIO_set_state(LCD_RS, 0);
	write(command);
}

void LCD_display_cursor(bool display, bool blink)
{
	uint8_t function = LCD_DISPLAY_ONOFF | LCD_DISPLAY_ON;
	function |= display ? LCD_CURSOR_ON : 0;
	function |= blink ? LCD_CURSOR_BLINK : 0;
	LCD_write_command(function);
}

static void write(uint8_t data)
{
	GPIO_set_state(LCD_E, 1);
	write_nibble(data >> 4);  // Write the most significant 4 bits first
	GPIO_set_state(LCD_E, 0);
	GPIO_set_state(LCD_E, 1);
	write_nibble(data);  // Write the remaining 4 bits
	GPIO_set_state(LCD_E, 0);

	struct timespec delay = {
		.tv_sec = 0,
		.tv_nsec = US_TO_NS(50),
	};
	nanosleep(&delay, NULL);
}

static void write_nibble(uint8_t nibble)
{
	GPIO_set_state(LCD_D4, nibble & 0x01);
	GPIO_set_state(LCD_D5, nibble & 0x02);
	GPIO_set_state(LCD_D6, nibble & 0x04);
	GPIO_set_state(LCD_D7, nibble & 0x08);
}