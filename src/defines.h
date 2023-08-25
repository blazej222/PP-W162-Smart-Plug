#pragma once
// GPIOs
#define RELAY_PIN                       14
#define SEL_PIN                         12
#define CF1_PIN                         5
#define CF_PIN                          4
#define BUTTON_PIN                      3
#define LIGHT_PIN                       13
// Check values every 5 seconds
#define UPDATE_TIME                     5000

// Set SEL_PIN to LOW to sample current, otherwise sample voltage
#define CURRENT_MODE                    LOW

// Mnozniki
/*
Current: 26506.00
Voltage: 265613.72
Power: 3540000.00
*/
#define CURRENT_RATIO             26506
#define VOLTAGE_RATIO             158227 //265613
#define POWER_RATIO               3540000