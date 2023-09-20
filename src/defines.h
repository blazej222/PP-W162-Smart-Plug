#pragma once
//Compilation options
#define DEBUG_BUILD
#define IGNORE_PHYSICAL_BUTTON
// GPIOs
#define RELAY_PIN                       14
#define SEL_PIN                         12
#define CF1_PIN                         5
#define CF_PIN                          4
#define BUTTON_PIN                      3
#define LED_PIN                         13
// Check values every 5 seconds
#define UPDATE_TIME                     5000

// Set SEL_PIN to LOW to sample current, otherwise sample voltage
#define CURRENT_MODE                    LOW

// Multipliers
/*
Current: 26506.00
Voltage: 265613.72
Power: 3540000.00
*/
#define CURRENT_RATIO             26506
#define VOLTAGE_RATIO             158227 //265613
#define POWER_RATIO               3540000

//Misc
#define WIFI_TIMEOUT 10000 //after this time in miliseconds if unable to connect to network, AP will be started
#define METERING_TIMEOUT 500000 //after this time in microseconds if power meter hasn't received any pulse, pulse value will be reset to 0

#ifdef DEBUG_BUILD
    #define debug_print(x) Serial.print(x)
#else
    #define debug_print(x)
#endif