# Fan controller

## Introduction
A simple ESP32 based fan controller used to extract hot air from my fire place.

## Hardware
- A [LILYGO T7-S3](https://github.com/Xinyuan-LilyGO/T7-S3) board is used (another ESP32 board may also do the job).
- Two Dallas temperature sensors (DS18B20) are present to measure inlet and outlet air temperature.
- Two PWM outputs are driven to set fan rotation speed
- A digital input can be used to force fan on (optionnal)
- A digital input can be used to force fan off (optionnal)

Here is a summary of microcontroller pinout (feel free to edit pin_config.h to change them!):

| Name        | Pin number | Description                         |
|-------------|------------|-------------------------------------|
|PIN_FAN1     | 17         | Fan 1 PWM output                    |
|PIN_FAN2     | 3          | Fan 2 PWM output                    |
|PIN_TEMP_1   | 15         | One wire bus of temperature probe 1 |
|PIN_TEMP_2   | 16         | One wire bus of temperature probe 2 |
|PIN_FORCE_ON | 9          | Input to force fan on (pull to GND) |
|PIN_FORCE_OFF| 11         | Input to force fan off (pull to GND)|


## Setup
Flash the firmware on the board.

A captive portal will be automatically shown at first usage. It allows you to select the wifi network and configure MQTT broker.  

After connected to wifi network, this configuration page can be reached at `http://xxxx/www/config.html` where xxxx is the IP/hostname of the ESP32.

## MQTT services
Here is the list of MQTT topics published/subscribed by the device.   
MQTT topic names are prefixed with a custom value set in the captive portal configuration page.

Subscribed topics (commands):
1. `/mode` : This service is used to change the operating mode. Here is the list of supported device modes:   
            - `Off` : Fan are off no matter the temperature measured   
            - `Forced` : Fan are on no matter the temperature measured   
            - `Auto` : Fan are switched on automatically, depending on measured temperature
            
2. `/percentage` : Sets the fans rotation speed in percent
3. `/auto_off_temp` : Sets the temperature at which the fan will stop (in automatic mode)
4. `/auto_on_temp` : Sets the temperature at which the fan will start (in automatic mode)
5. `/set` : Write to `on` or `true` to change mode to *Forced* or *Off*

Published topics (acquisition):
1. `/status` : This topic will publish a JSON payload containing following objects:   
```json
{
    "percentage" : 100.0,
    "temperature1" : 24.0,
    "temperature2" : 26.0,
    "offTemperature" : 20.0,
    "onTemperature" : 24.0,
    "mode" : "Auto",
    "state" : "off"
}
```
Here is some details about fields contained in this JSON data:
- `percentage` : Fans rotation speed percentage when on
- `temperature1` : Actual temperature measured by probe 1 (inlet)
- `temperature2` : Actual temperature measured by probe 2 (outlet)
- `offTemperature` : Temperature when fan will be switched off (in *Auto* mode)
- `onTemperature` : Temperature when fan will be switched on (in *Auto* mode)
- `mode` : Actual operating mode (*Auto*, *Forced*, *Off*)
- `state` : State of the fan (*on* or *off*)
