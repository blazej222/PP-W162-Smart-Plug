#pragma once
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include "powerMeter.h"

extern ESP8266WebServer server;
extern ESP8266HTTPUpdateServer updater;

extern String mainpage;
extern String settingspage;
extern String devicename;
extern uint8_t rate; //page refresh rate

extern bool relaystatus;
extern bool autoenable;
extern bool measureVoltage;
extern bool measureCurrent;

extern String SSID;
extern String KEY;

extern void generateNewSettings();
extern void enableRelay();
extern void disableRelay();

extern PowerMeter meter;

extern unsigned int energySendingFrequency;
extern unsigned int statTime;
extern unsigned int statSendingFrequency;
extern IPAddress serveraddr;
extern uint16_t port;

void handle_enable();
void handle_disable();
void handle_root();
void initServer();