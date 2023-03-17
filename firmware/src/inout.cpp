#include "inout.h"
#include "pin_config.h"
#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire temp1OneWire(PIN_TEMP_1);
static DallasTemperature temp1Sensor(&temp1OneWire);

static OneWire temp2OneWire(PIN_TEMP_2);
static DallasTemperature temp2Sensor(&temp2OneWire);

/**
 * Configure in/out ports
*/
void setup_inputs_outputs()
{
    //Configure pins for PWM
    //Configure PWM with a frequency of 25Khz
    ledcSetup(1, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_FAN1, 1);
    ledcSetup(2, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_FAN1, 2);

    //Led for captive portal
    if(PIN_USER_LED >= 0){
        pinMode(PIN_USER_LED, OUTPUT);
    }

    //Temperature sensors
    temp1Sensor.begin();
    temp2Sensor.begin();
}

void set_fan_speed(int channel, double percent)
{
    if((percent >= 0.0) && (percent <= 100.0)){
        if((channel > 0) && (channel < 3)){
            uint32_t duty = (uint32_t)(percent * (((1 << PWM_RESOLUTION_BITS) - 1)/100.0));
            ledcWrite(channel, duty);
        }
    }
}

double getTemperature1()
{
    temp1Sensor.requestTemperatures();
    return temp1Sensor.getTempCByIndex(0);
}

double getTemperature2()
{
    temp2Sensor.requestTemperatures();
    return temp2Sensor.getTempCByIndex(0);
}