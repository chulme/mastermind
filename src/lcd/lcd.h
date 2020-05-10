#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialises the LCD display
 * If init_gpio is true, the function will
 * also initialise the GPIO module.
*/
void LCD_init(bool init_gpio);

/**
 * Clears the entire display
*/
void LCD_clear(void);

/**
 * Sets the "cursor" to the x, y position
*/
void LCD_go_to(uint8_t x, uint8_t y);

/**
 * Writes ASCII characters to the display
*/
void LCD_write_text(char *text);

/**
 * Writes a single character to the display
*/
void LCD_write_data(uint8_t data);

/**
 * Writes a command to the display
*/
void LCD_write_command(uint8_t command);

/**
 * Cursor settings
 * display - turn the cursor on (true) or off (false)
 * blink - enable blinking when blink=true, disable if false
*/
void LCD_display_cursor(bool display, bool blink);
#endif