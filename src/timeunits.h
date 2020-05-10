#ifndef TIMEUNITS_H
#define TIMEUNITS_H

/**
 * This header file provides pre-processor macros for time unit conversion.
*/

/**
 * Converts seconds to nanoseconds
*/
#define SEC_TO_NS(x) ((x) * 1000000000)

/**
 * Converts miliseconds to nanoseconds
*/
#define MS_TO_NS(x) ((x) * 1000000)

/**
 * Converts microseconds to nanoseconds
*/
#define US_TO_NS(x) ((x) * 1000)

#endif