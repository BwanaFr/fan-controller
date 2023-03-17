#ifndef _INOUT_H_
#define _INOUT_H_
#include <cstdint>


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
*/
double getTemperature1();

/**
 * Gets temperature 2 sensor
*/
double getTemperature2();


#endif