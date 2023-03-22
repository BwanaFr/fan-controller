#ifndef _INOUT_H_
#define _INOUT_H_
#include <cstdint>

#define TEMP_RESOLUTION 12

/**
 * Setups the I/O (digital and analog)
*/
void setup_inputs_outputs();

/**
 * Sets fan speed
 * @param channel Fan channel (1,2)
 * @param percent Fan percentage (0-100)
*/
void set_fan_speed(int channel, double percent);

/**
 * Gets temperature 1 sensor
 * @return temperature 1 sensor
*/
double getTemperature1();

/**
 * Gets temperature 2 sensor
 * @return temperature 2 sensor
*/
double getTemperature2();

/**
 * Gets the forced (on) input status
 * @return True when the input is active
*/
bool getForcedInput();

/**
 * Gets the off input status
 * @return True when the input is active
*/
bool getOffInput();


#endif