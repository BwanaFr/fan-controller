#include "inout.h"
#include "pin_config.h"
#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire temp1OneWire(PIN_TEMP_1);
static DallasTemperature temp1Sensor(&temp1OneWire);
static DeviceAddress temp1DeviceAddress;
static unsigned long lastTemp1Request = 0;
static float temperature1 = 0.0;

static OneWire temp2OneWire(PIN_TEMP_2);
static DallasTemperature temp2Sensor(&temp2OneWire);
static DeviceAddress temp2DeviceAddress;
static unsigned long lastTemp2Request = 0;
static float temperature2 = 0.0;

static int  delayInMillis = 0;

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
    temp1Sensor.getAddress(temp1DeviceAddress, 0);
    temp1Sensor.setResolution(temp1DeviceAddress, TEMP_RESOLUTION);
    temp1Sensor.setWaitForConversion(false);
    temp1Sensor.requestTemperatures();
    delayInMillis = 750 / (1 << (12 - TEMP_RESOLUTION)); 
    lastTemp1Request = millis(); 

    temp2Sensor.begin();
    temp2Sensor.getAddress(temp2DeviceAddress, 0);
    temp2Sensor.setResolution(temp2DeviceAddress, TEMP_RESOLUTION);
    temp2Sensor.setWaitForConversion(false);
    temp2Sensor.requestTemperatures();
    lastTemp2Request = millis(); 
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
    unsigned long now = millis();
    if (now - lastTemp1Request >= delayInMillis) // waited long enough??
    {
        temperature1 = temp1Sensor.getTempCByIndex(0);
        temp1Sensor.requestTemperatures();
        lastTemp1Request = now;
    }
    return temperature1;
}

double getTemperature2()
{
    unsigned long now = millis();
    if (now - lastTemp2Request >= delayInMillis) // waited long enough??
    {
        temperature2 = temp2Sensor.getTempCByIndex(0);
        temp2Sensor.requestTemperatures();
        lastTemp2Request = now;
    }
    return temperature2;
}