#include "gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "../timeunits.h"

#define BCM2708_PERI_BASE 0x3F000000
#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

#define SUCCESS 0
#define FAILURE -1

#define BTN_TIMEOUT_S 2  // seconds
#define BTN_PROBE_TIME_MS 100
#define BOUNCE_TIME_MS 30


static void *gpio_map;

// I/O access
static volatile unsigned int *gpio;

int GPIO_init(void)
{
	int mem_fd;
	if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
	{
		perror("Can't open /dev/mem");
		return FAILURE;
	}

	// mmap GPIO
	gpio_map = mmap(
		NULL,  // Any adddress in our space will do
		BLOCK_SIZE,  // Map length
		PROT_READ | PROT_WRITE,  // Enable reading & writting to the mapped memory
		MAP_SHARED,  // Shared with other processes
		mem_fd,  // File to map
		GPIO_BASE  // Offset to GPIO peripheral
	);

	close(mem_fd);

	if (gpio_map == MAP_FAILED)
	{
		fprintf(stderr, "mmap error %d\n", (int)gpio_map);
		return FAILURE;
	}

	gpio = (volatile unsigned int *)gpio_map;
	return SUCCESS;
}

void GPIO_set_in(uint8_t pin)
{
	if (!gpio)
	{
		fprintf(stderr, "Error: Null GPIO pointer\n");
		return;
	}

	asm volatile
	(
		/* Get the function select register address offset
		 * by calculating the remainder of the pin number
		 * when dividing by 10.
		 * The division is done by repeated subtraction.
		*/

		// Setup
		"MOV R8, %[pin]\n"
		"MOV R9, #0\n"  // Quotient
		"MOV R10, R8\n"  // Remainder
		"MOV R12, #10\n"  // Divisor
		"B cond\n"  // Go to loop condition
		// Division loop
		"div:\n"
		"SUBS R10, R10, R12\n" // remainder = remainder - divisor
		"ADDPL R9, #1\n"  // If the result of the subtraction is not negative, add 1 to the quotient
		"cond:\n"
		"CMP R10, R12\n"  // Check if the remainder can be divided (i.e. >= 10)
		"BHS div\n"  // If R10 >= 10, divide again.

		// At this point R9 is the offset multiplier for the function register.
		// Multiply by 4 to get the right offset
		"MOV R12, #4\n"
		"MOV R8, R9\n"
		"MUL R9, R8, R12\n"
		// Reset the 3 bits that belong to the pin function.
		"MOV R14, %[gpio]\n"  // Get the base address of GPIO into R14
		"LDR R4, [R14, R9]\n"  // Load contents of at GPIO+R9 (function select register)
		"MOV R3, #3\n"  // Multiplier for the shift (number of bits per function select of a pin)
		"MOV R5, #7\n"  // Reset bit mask
		"MUL R6, R10, R3\n"  // Multiply reminder by 3 (R3) to get the correct shift multiplier
		"LSL R5, R6\n"  // Shift the mask
		"BIC R4, R4, R5\n"  // Clear the bits belonging to that pin
		"STR R4, [R14, R9]\n"
		:
		:[pin]"r"(pin),
		 [gpio]"r"(gpio)
		:"memory"
	);
}

void GPIO_set_out(uint8_t pin)
{
	if (!gpio)
	{
		fprintf(stderr,"Error: Null GPIO pointer\n");
		return;
	}

	GPIO_set_in(pin);
	asm volatile
	(
		"MOV R5, #1\n"  // 1 is output
		"LSL R5, R6\n"  // Shift the output bit to the correct position of that pin
		"ORR R4, R5\n"  // Set the pin as output
		"STR R4, [R14, R9]\n"  // Store the new contents of the register
		:
		:[pin]"r"(pin),
		 [gpio]"r"(gpio)
		:"memory"
	);
}

void GPIO_set_state(uint8_t pin, uint8_t state)
{
	if (state)
	{
		asm volatile
		(
			"MOV R5, %[gpio]\n"  // Get the base address of GPIO
			"LDR R6, [R5, #0x1C]\n"  // Get the contents of GPSET0 register (offset 0x1C or 28 dec.)
			"MOV R7, #1\n"  // 1 for setting the state
			"LSL R7, %[pin]\n"  // pin number == shift amount
			"STR R7, [R5, #0x1C]\n"  // Store the new contents of the register
			:
			:[pin]"r"(pin),
			 [gpio]"r"(gpio)
			:"memory"
		);
	}
	else
	{
		asm volatile
		(
			// Similar to when state > 0, except the register is GPCLR0 (offset 0x28 or 40 dec.)
			"MOV R5, %[gpio]\n"
			"LDR R6, [R5, #0x28]\n"
			"MOV R7, #1\n"
			"LSL R7, %[pin]\n"
			"STR R7, [R5, #0x28]\n"
			:
			:[pin]"r"(pin),
			 [gpio]"r"(gpio)
			:"memory"
		);
	}
}

/**
 * Returns the state of pin
 * 0 - low, 1 - high
*/
static uint8_t get_state(uint8_t pin)
{
	uint32_t state = 0;

	asm volatile
	(
		"MOV R5, %[gpio]\n"  // Get the base address of GPIO
		"LDR R6, [R5, #0x34]\n"  // Get the contents of GPLEV0 register
		"MOV R7, %[pin]\n"  // Get the pin number, used for shifting
		"MOV R8, #1\n"
		"LSL R8, R7\n" // Shift the 1 to the right place
		"AND %[state], R6, R8\n"
		:[state]"=r"(state)
		:[pin]"r"(pin),
		 [gpio]"r"(gpio)
	);

	return state > 0;
}

uint8_t GPIO_get_state(uint8_t pin)
{
	struct timespec delay =
	{
		.tv_sec = 0,
		.tv_nsec = MS_TO_NS(BOUNCE_TIME_MS),
	};
	if (get_state(pin))  // Check if the button is pressed
	{
		nanosleep(&delay, NULL);  // Wait BOUNCE_TIME_MS and check again
		if (get_state(pin))
			return 1; // If it's still pressed (button state stabilized), return 1
	}
	return 0;
}

uint8_t GPIO_get_button_press(uint8_t pin)
{
	while (GPIO_get_state(pin) == 0)
		;  // Wait for the button to be pressed
	while (GPIO_get_state(pin) != 0)
		;  // Wait for the button to be released

	return 1;
}

uint8_t GPIO_get_button_presses(uint8_t pin, uint8_t max, void (*click_handler)(uint8_t presses))
{
	GPIO_get_button_press(pin);

	uint8_t presses = 1;  // Now, the button has been pressed once

	if (click_handler)
		click_handler(presses);

	uint32_t timeout = SEC_TO_NS(BTN_TIMEOUT_S);
	uint32_t probe_time = MS_TO_NS(BTN_PROBE_TIME_MS);

	struct timespec delay =
	{
		.tv_sec = 0,
		.tv_nsec = probe_time,
	};

	for (uint32_t i = 0; i < timeout; i += probe_time)
	{
		if (GPIO_get_state(pin) == 0)
		{
			nanosleep(&delay, NULL);
			delay.tv_nsec = probe_time;
			continue;
		}

		// Button has been pressed
		i = 0;  // Reset the timeout counter
		presses++;
		if (presses > max)  // wrap around
			presses = 1;

		if (click_handler)
			click_handler(presses);

		// Wait for the button to be released
		while (GPIO_get_state(pin) != 0)
			;

	}
	return presses;
}
