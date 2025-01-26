#pragma once
//Compilation options
// #define DEBUG_BUILD
// #define IGNORE_PHYSICAL_BUTTON
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
#define CURRENT_RATIO             13259
#define VOLTAGE_RATIO             134147 
#define POWER_RATIO               1674421

//Misc
#define WIFI_TIMEOUT 10000 //after this time in miliseconds if unable to connect to network, AP will be started
#define METERING_TIMEOUT 500000 //after this time in microseconds if power meter hasn't received any pulse, pulse value will be reset to 0
#define LED_BLINK_EVERY 10000
#define LED_BLINK_FOR 300
#define METER_MODESWAP_TIMEOUT 800 //miliseconds
#define NTP_SERVER "pl.pool.ntp.org"
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

#ifdef DEBUG_BUILD
    #define debug_print(x) Serial.print(x)
#else
    #define debug_print(x)
#endif