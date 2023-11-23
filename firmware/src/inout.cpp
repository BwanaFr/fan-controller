#include "inout.h"
#include "pin_config.h"
#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

class DallasTemperatureAsync {
private:
    DallasTemperature sensor;
    OneWire oneWire;
    DeviceAddress deviceAddress;
    unsigned long lastTempRequest;
    float temperature;
    int delayInMillis;
public:    
    DallasTemperatureAsync(uint8_t pin) :  oneWire(pin), sensor(&oneWire)
    {
        sensor.begin();
        sensor.getAddress(deviceAddress, 0);
        sensor.setResolution(deviceAddress, TEMP_RESOLUTION);
        sensor.setWaitForConversion(false);
        sensor.requestTemperatures();
        delayInMillis = 750 / (1 << (12 - TEMP_RESOLUTION)); 
        lastTempRequest = millis(); 
    }
    virtual ~DallasTemperatureAsync(){}

    float getTemperature(){
        unsigned long now = millis();
        if (now - lastTempRequest >= delayInMillis) // waited long enough??
        {
            temperature = sensor.getTempCByIndex(0);
            sensor.requestTemperatures();
            lastTempRequest = now;
        }
        return temperature;
    }
};


static DallasTemperatureAsync temp1Sensor(PIN_TEMP_1);
static DallasTemperatureAsync temp2Sensor(PIN_TEMP_2);

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
    ledcAttachPin(PIN_FAN2, 2);

    //Led for captive portal
    if(PIN_USER_LED >= 0){
        pinMode(PIN_USER_LED, OUTPUT);
    }

    //User input
    pinMode(PIN_FORCE_ON, INPUT_PULLUP);
    pinMode(PIN_FORCE_OFF, INPUT_PULLUP);
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
    return temp1Sensor.getTemperature();
}

double getTemperature2()
{
    return temp2Sensor.getTemperature();
}

bool getForcedInput()
{
    return !digitalRead(PIN_FORCE_ON);
}

bool getOffInput()
{
    return !digitalRead(PIN_FORCE_OFF);
}