# Mastermind
To purpose of this project is to create the game Mastermind, using the C programming language and inline ARM assembly, executing on the Raspberry Pi with external input/output devices.

This project has been completed by Adam Malek and Christopher Hulme as a university project.

## Game Rules
MasterMind is a two player game between a codemaker and a codebreaker. Before the game, a sequence length of N and a number of C colours for an arbitrary number of pegs are fixed. Then the codemaker selects N pegs and places them into a sequence of N slots. This (hidden) sequence comprises the code that should be broken. In turns, the codebreaker tries to guess the hidden sequence, by composing a sequence of N coloured pegs, choosing from the C colours.

In each turn the codemaker answers by stating how many pegs in the guess sequence are both of the right colour and at the right position, and how many pegs are of the right colour but not in the right position. The codebreaker uses this information in order to refine his guess in the next round. The game is over when the codebreaker successfully guesses the code, or
if a fixed number of turns has been played.

Below is a sample sequence of moves (R red, G green, B blue) for the board-game:
Secret: R G G
================
Guess1: B R G
Answ1: 1 1
Guess2: R B G
Answ2: 2 0
Guess3: R G G
Answ3: 3 0
Game finished in 3 moves.

For details see the MasterMind Wikipedia page:
https://en.wikipedia.org/wiki/Mastermind_%28board_game%29

## Download and Installation
The program has to be executed with sudo privileges: `sudo build/mastermind`

Objects and the executable can be removed with the command: `make clean`


Compilation can also be done with make: `make all`
## Implementation 
### Hardware
A Raspberry Pi 2 was used in conjunction with an external breadboard circuit featuring 2 LEDs, a button, a potentiometer and a 16x2 LCD screen.

The breadboard circuit follows the wiring diagram provided. Following the wiring diagram, the RPi GPIO pin 13 was used for the green LED, pin 5 was used for the red LED and pin 19 for the button.

<img src="https://i.imgur.com/amXT0Cm.png" width="525">

### Code
The code is split into 3 modules
* GPIO –for controlling the GPIO pins using inline assembly.
* LCD – for controlling the LCD display.
* Mastermind – implements the gameplay logic and brings GPIO and LCD modules together
