#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "timeunits.h"

#include "gpio/gpio.h"
#include "lcd/lcd.h"

#define LED_G 13
#define LED_R 5
#define BTN 19

#define SEC 1000000
#define HALF_SEC 500000

#define SETTINGS 3
#define ARG_CHARACTERS 4
#define DESC_MAX_LENGTH 40

// Default number of numbers (sequence length)
#define NUMBERS_DEF 3
// Default number of rounds
#define ROUNDS_DEF 3
// Default maximum number
#define MAX_DEF 3

// Game settings with default values
static uint8_t number_of_numbers = NUMBERS_DEF;
static uint8_t number_of_rounds = ROUNDS_DEF;
static uint8_t max_random = MAX_DEF;
static bool debug = false;

struct setting
{
	char arg[ARG_CHARACTERS];  // argument string, for example "-n="
	uint8_t *value;  // variable value to change
	char description[DESC_MAX_LENGTH];  // description of the setting
};

static uint8_t cursor_x = 0;  // Keep track of where the cursor is for input

/**
 * This function will be called by GPIO_get_button_presses on every button press
*/
void MM_handle_button_press(uint8_t presses)
{
	if (presses > 99)  // buffer of 3 characters can only fit 2 digits
	{
		fprintf(stderr, "Warning - presses does not fit into the buffer. \
						Nothing will be diplayed on the LCD");
		return;
	}
	char buff[3];
	sprintf(buff, "%d", presses);
	LCD_write_text(buff);
	LCD_go_to(cursor_x, 0);  // The cursor will move 2 characters to the right, move it back
}

void MM_flash_led(const int led, const int number_of_flashes)
{
	for (int i = 0; i < number_of_flashes; i++)
	{
		GPIO_set_state(led, 1);
		usleep(HALF_SEC);
		GPIO_set_state(led, 0);
		usleep(HALF_SEC);
	}
}

static void MM_get_one_number(int *input)
{
	LCD_display_cursor(true, true);  // Enable blinking cursor
	*input = GPIO_get_button_presses(BTN, max_random, MM_handle_button_press);
	cursor_x += 2;  // Update the cursor position
	LCD_go_to(cursor_x, 0);  // Move the cursor to that position
	LCD_display_cursor(true, false);  // Display cursor but don't blink
}

/**
 * Use LEDs to acknowledge the input
*/
static void MM_acknowledge_input(const int presses)
{
	MM_flash_led(LED_R, 1);
	MM_flash_led(LED_G, presses);
}

/**
 * Flash the red LED to represent the end of input
*/
static void MM_end_input(void)
{
	LCD_display_cursor(false, false);  // Turn off the cursor
	MM_flash_led(LED_R, 2);  // End of sequence flash
	cursor_x = 0;
}

/**
 * Returns a pointer to an integer array with the user input
*/
int *MM_get_guess(void)
{
	int *input = malloc(number_of_numbers * sizeof(int));
	if (!input)
	{
		perror("Unable to allocate memory for the input");
		return NULL;
	}

	for (int i = 0; i < number_of_numbers; i++)
	{
		MM_get_one_number(&input[i]);
		MM_acknowledge_input(input[i]);
	}
	MM_end_input();
	return input;
}

/**
 * Calculates the number of exact and approximate (wrong position) matches.
*/
void MM_calculate_matches(int *exact, int *approx, int secret[], int guess[], size_t size)
{
	/*
	 * The algorithm creates a bool array of size N.
	 * This is done to solve the issue where a loop
	 * for approximate matches would count the same
	 * number twice.
	*/
	bool *counted = calloc(size, sizeof(bool));  // Create the array, initialised with 0s (false)

	for (size_t i = 0; i < size; i++)
	{
		if (guess[i] == secret[i])
		{
			(*exact)++;
			/*
			 * It may happen that secret[i] was already checked and counted as an approximate match
			 * Therefore, if the counted flag at position i is true we need to decrement the number
			 * of approximate matches because this number will be actually an exact match.
			*/
			if (counted[i])
				(*approx)--;

			counted[i] = true;
		}
		else
		{
			// Check if the number exists somewhere else in the array
			for (size_t j = 0; j < size; j++)
			{
				if (guess[i] == secret[j] && !counted[j])  // If the same and not already counted
				{
					counted[j] = true;
					(*approx)++;
					break;  // Found a match for guess[i] - exit the loop and get the next guess number
				}
			}
		}
	}

	free(counted);
}

/**
 * Initialises the GPIO and LCD modules.
 * Returns true on success
*/
bool MM_init()
{
	if (GPIO_init() != 0)  // 0 means success
		return false;
	GPIO_set_out(LED_G);
	GPIO_set_out(LED_R);
	GPIO_set_in(BTN);
	GPIO_set_state(LED_G, 0);
	GPIO_set_state(LED_R, 0);
	LCD_init(false);
	LCD_go_to(0, 0);

	return true;
}

/**
 * Output on a failed guess
*/
void MM_attempt_output(int approx, int exact)
{
	LCD_clear();

	// LCD Exact output
	LCD_go_to(0, 0);
	char exact_buffer[10];
	sprintf(exact_buffer, "Exact: %d", exact);
	LCD_write_text(exact_buffer);

	// LCD approx output
	LCD_go_to(0, 1);
	char approx_buffer[10];
	sprintf(approx_buffer, "Approx: %d", approx);
	LCD_write_text(approx_buffer);

	// Flashes for number of exact matches
	MM_flash_led(LED_G, exact);
	// Flashes red once as seperator
	MM_flash_led(LED_R, 1);
	// Flashes for number of approx matches
	MM_flash_led(LED_G, approx);
}

/**
 * Output on a correct guess
*/
void MM_success_output(int number_of_rounds)
{
	usleep(SEC);
	LCD_clear();
	LCD_go_to(0, 0);
	LCD_write_text("Success!");

	// Display the number of played rounds
	LCD_go_to(0, 1);
	char rounds_buffer[16];
	sprintf(rounds_buffer, "Rounds: %d", number_of_rounds);
	LCD_write_text(rounds_buffer);

	GPIO_set_state(LED_R, 1);
	usleep(HALF_SEC);
	MM_flash_led(LED_G, 3);
	usleep(HALF_SEC);
	GPIO_set_state(LED_R, 0);
}

/**
 * Returns pseudo-randomly generated secret
 * for user to guess
*/
int *MM_generate_secret()
{
	int *secret = malloc(number_of_numbers * sizeof(int));
	srand(time(NULL));

	for (int i = 0; i < number_of_numbers; i++)
	{
		secret[i] = (rand() % max_random) + 1;
	}
	return secret;
}

static void MM_output_numbers(char *message, int *array, size_t array_size)
{
	printf("%s:", message);
	for (size_t i = 0; i < array_size; i++)
	{
		printf(" %d", array[i]);
	}
	printf("\n");
}

/**
 * Returns true if the given argument is the debug argument
*/
static bool MM_debug_arg(char *arg)
{
	return strcmp(arg, "-d") == 0;
}

/**
 * Enables the debug flag, changes the game settings to default.
*/
static void MM_enable_debugging(void)
{
	debug = true;
	number_of_numbers = NUMBERS_DEF;
	number_of_rounds = ROUNDS_DEF;
	max_random = MAX_DEF;
	printf("Warning - Debugging enabled, default settings will be used. "
		   "Other arguments will be ignored\n");
}

/**
 * Returns true if the given argument was succesfully parsed with the given setting
*/
static bool MM_parse_setting_arg(char *arg, struct setting setting)
{
	/*
	 * We expect all arguments to be in the form "-[c]=[d]" where [c] is a character,
	 * for example "-n=" and [d] is an unsigned number.
	 * Therefore, we compare the first 3 characters of the argument string
	 * and then check if the string is longer than 3 character (if there is a number after "-[c]=")
	*/
	if (strncmp(arg, setting.arg, 3) == 0 && strlen(arg) > 3)
	{
		// Ignore 3 characters, then parse uint8_t
		if (sscanf(arg, "%*c%*c%*c%hhu", setting.value))
		{
			// Display Success message
			printf("%s changed to %hhu\n", setting.description, *setting.value);
			return true;
		}
		// sscanf failed, possibly because the string after "-[c]=" is not a number
		printf("Error - Parsing %s failed. %s not changed, using the default value (%hhu).\n",
				arg, setting.description, *setting.value);
	}

	return false;
}

/**
 * Goes through the defined (inside) settings structure array and tries to parse the arg
*/
static void MM_parse_settings(char *arg)
{
	struct setting settings[SETTINGS] =
	{
		{"-n=", &number_of_numbers, "Number of numbers (sequence length)"},
		{"-c=", &max_random, "Maximum number"},
		{"-r=", &number_of_rounds, "Number of rounds"},
	};

	for (uint8_t i = 0; i < SETTINGS; i++)
	{
		if (MM_parse_setting_arg(arg, settings[i]))
			return;  // Succesfully parsed the argument
	}

	// No settings arg strings match the given arg - display error
	fprintf(stderr, "Error - Argument %s is invalid.\n", arg);
}

/**
 * Parses the program arguments
*/
static void MM_parse_args(int argc, char *argv[])
{
	// Go through all of the given arguments except the first one (name of the program)
	for (int i = 1; i < argc; i++)
	{
		if (MM_debug_arg(argv[i]))
		{
			MM_enable_debugging();

			// When debugging we use the default values, so we don't care about other args.
			return;
		}

		MM_parse_settings(argv[i]);
	}
}

int main(int argc, char *argv[])
{
	printf("Welcome to Mastermind, coded by Adam Malek & Chris Hulme for Hardware-Software Interface.\n");

	if (!MM_init())
	{
		fprintf(stderr, "Failed to initialise the game. This program has to be run with sudo privileges\n");
		exit(EXIT_FAILURE);
	}
	MM_parse_args(argc, argv);
	int *secret = MM_generate_secret();

	if (debug)
	{
		MM_output_numbers("Secret", secret, number_of_numbers);
	}

	bool success = false;  // Indicates whether the guess was successful (true) or not

	// The following loop will repeat for the number of rounds or until the guess was successful
	for (int i = 1; i <= number_of_rounds && !success; i++)
	{
		int *guess = MM_get_guess();

		if (debug)
		{
			MM_output_numbers("Guess", guess, number_of_numbers);
		}

		int exact = 0;
		int approximate = 0;
		MM_calculate_matches(&exact, &approximate, secret, guess, number_of_numbers);

		// Successful guess
		if (exact == number_of_numbers)
		{
			MM_success_output(i);
			success = true;
		}
		else
		{
			MM_attempt_output(approximate, exact);
			printf("Press the button to continue...\n");
			LCD_display_cursor(true, true);
			GPIO_get_button_press(BTN);
			LCD_display_cursor(true, false);
			MM_flash_led(LED_R, 3);
			LCD_clear();
		}
		free(guess);
	}

	if (!success)  // Only executed if the user failed to guess the secret
	{
		LCD_clear();
		LCD_display_cursor(false, false);
		LCD_go_to(0, 0);
		LCD_write_text("GAME OVER");
	}

	free(secret);
	return EXIT_SUCCESS;
}