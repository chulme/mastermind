#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

/**
 * Initialises the GPIO module.
 * Returns 0 on success, -1 on failure.
*/
int GPIO_init(void);

/**
 * Sets pin as input
*/
void GPIO_set_in(uint8_t pin);

/**
 * Sets pin as output
*/
void GPIO_set_out(uint8_t pin);

/**
 * Changes the state of the pin
 * 0 - low, 1 - high
*/
void GPIO_set_state(uint8_t pin, uint8_t state);

/**
 * Returns the state of the pin (with debouncing)
 * 0 - low, 1 - high
*/
uint8_t GPIO_get_state(uint8_t pin);

/**
 * Waits for a press of the button (high level of the pin).
 * Returns 1 after the button is released (low level).
*/
uint8_t GPIO_get_button_press(uint8_t pin);

/**
 * Returns the number of times the button has been pressed (at least once).
 * The function waits for the first button press and after it is released
 * it runs a timeout loop during which a button press will reset it.
 * The max is for wrap around - if the number of presses exceeds the max
 * it is reset to 1.
 *
 * The click_handler is a pointer to void function which accepts uint8_t data type as a parameter
 * The handler will be called on every button press and the value passed will be the current
 * number of presses (with wrap around)
*/
uint8_t GPIO_get_button_presses(uint8_t pin, uint8_t max, void (*click_handler)(uint8_t presses));

#endif

