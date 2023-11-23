#pragma once

#define PWM_RESOLUTION_BITS 8
#define PWM_FREQUENCY 25000

/* Fan 1 PWM output*/
#define PIN_FAN1 3
/* Fan 2 PWM output */
#define PIN_FAN2 4
/* Temperature 1 one wire bus */
#define PIN_TEMP_1 15
/* Temperature 2 one wire bus */
#define PIN_TEMP_2 16
/* User LED (onboard, IO17) */
#define PIN_USER_LED 17
/* Force on fans (mode forced) input */
#define PIN_FORCE_ON 9
/* Force on fans (mode OFF) input */
#define PIN_FORCE_OFF 11