#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "defines.h"
#include "powerMeter.h"
#include "webserver.h"
#include "comms.h"

extern ESP8266WebServer server;

PowerMeter meter(RELAY_PIN,SEL_PIN,CF1_PIN,CF_PIN);

// Wifi settings
String SSID;
String KEY;
String deviceName;

extern uint8_t mainWebsiteRefreshRate; //page refresh rate
unsigned long lastTimeStatCollected = 0;
unsigned long lastTimeEnergySent = 0;
unsigned long lastTimeButtonStateChanged = 0;
unsigned long lastTimeLEDBlink = 0;
bool enableRelayOnPowerUp = false;
bool measureVoltage = true;
bool measureCurrent = false;
unsigned short LEDmode = 1; // 0 is disabled, 1 is as relay, 2 is blink every 10 seconds
bool blinkPhase = false;
extern time_t timeOfLastMeterReset;

void timeSetCallback(){
if(timeOfLastMeterReset <= 1714143023) time(&timeOfLastMeterReset);
}

void IRAM_ATTR cf1IRQ(){
  meter.cf1Interrupt();
}

void IRAM_ATTR cfIRQ(){
  meter.cfInterrupt();
}

void enableRelay(){
    debug_print("Enabling relay \n");
    relayStatus = true;
    digitalWrite(RELAY_PIN,HIGH);
    if(LEDmode == 1)digitalWrite(LED_PIN,LOW);
}

void disableRelay(){
    debug_print("Disabling relay \n");
    relayStatus = false;
    digitalWrite(RELAY_PIN,LOW);
    digitalWrite(LED_PIN,HIGH);
}

void initWifi(){
  debug_print("SSID:");
  debug_print(SSID);
  debug_print("\n");
  debug_print("Pass:");
  debug_print(KEY);
  debug_print("\n");
  WiFi.begin(SSID,KEY);
  unsigned long t1 = millis();
  while (WiFi.status() != WL_CONNECTED){
    yield();
    if(millis()-t1 > WIFI_TIMEOUT){ //if connection cannot be made within reasonable time
      WiFi.disconnect();
      IPAddress local_IP(192,168,1,1);
      IPAddress gateway(192,168,1,1);
      IPAddress subnet(255,255,255,0);
      WiFi.softAPConfig(local_IP, gateway, subnet);
      debug_print("Cannot connect to network, creating own AP\n");
      if(WiFi.softAP("Energy Meter","root123root123")) debug_print("Created AP succesfully.\n");
      else debug_print("Creating AP failed\n");
      break;
    }
  }; // hang until connection can be made
  debug_print("Wifi initialized\n");
  debug_print("WifiAddr is:");
  debug_print(WiFi.localIP());
  debug_print('\n');
}

void readSettings(){
  if(!LittleFS.begin())debug_print("LittleFS intialization failure \n");
  //delay(1000);
  File settings = LittleFS.open("/config.cfg","r");
  unsigned int currentMultiplier = settings.readStringUntil('\n').toInt();
  unsigned int voltageMultiplier = settings.readStringUntil('\n').toInt();
  unsigned int powerMultiplier = settings.readStringUntil('\n').toInt();
  meter.setMultipliers(voltageMultiplier,powerMultiplier,currentMultiplier);
  mainWebsiteRefreshRate = settings.readStringUntil('\n').toInt();
  SSID = settings.readStringUntil('\n');
  KEY = settings.readStringUntil('\n');
  enableRelayOnPowerUp = settings.readStringUntil('\n').toInt();
  deviceName = settings.readStringUntil('\n');
  measureVoltage = settings.readStringUntil('\n').toInt();
  measureCurrent = settings.readStringUntil('\n').toInt();
  meter.setSwapWait(settings.readStringUntil('\n').toInt());
  energySendingFrequency = settings.readStringUntil('\n').toInt();
  statCollectingFrequency = settings.readStringUntil('\n').toInt();
  statSendingFrequency = settings.readStringUntil('\n').toInt();
  if(statSendingFrequency != 0) stats = new CollectedStats(statSendingFrequency);
  dataCollectingServerIP.fromString(settings.readStringUntil('\n'));
  dataCollectingServerPort = settings.readStringUntil('\n').toInt();
  LEDmode = settings.readStringUntil('\n').toInt();
  settings.close();
  debug_print("Settings read! \n");
}

void generateNewSettings(){
  String newSettings = String(meter.getCurrentMultiplier());
  newSettings += '\n';
  newSettings += String(meter.getVoltageMultiplier());
  newSettings += '\n';
  newSettings += String(meter.getPowerMultiplier());
  newSettings += '\n';
  newSettings += String(mainWebsiteRefreshRate);
  newSettings += '\n';
  newSettings += SSID;
  newSettings += '\n';
  newSettings += KEY;
  newSettings += '\n';
  newSettings += enableRelayOnPowerUp;
  newSettings += '\n';
  newSettings += deviceName;
  newSettings += '\n';
  newSettings += measureVoltage;
  newSettings += '\n';
  newSettings += measureCurrent;
  newSettings += '\n';
  newSettings += meter.getSwapWait();
  newSettings += '\n';
  newSettings += energySendingFrequency;
  newSettings += '\n';
  newSettings += statCollectingFrequency;
  newSettings += '\n';
  newSettings += statSendingFrequency;
  newSettings += '\n';
  newSettings += dataCollectingServerIP.toString();
  newSettings += '\n';
  newSettings += dataCollectingServerPort;
  newSettings += '\n';
  newSettings += LEDmode;
  File settings = LittleFS.open("/config.cfg","w");
  settings.write(newSettings.c_str());
  settings.close();
  debug_print("New settings saved! \n");
}

void setup() {
  pinMode(RELAY_PIN,OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  pinMode(BUTTON_PIN,INPUT_PULLUP);

  #ifdef DEBUG_BUILD //enable Serial output for debuggind
    Serial.begin(115200);
  #endif

  debug_print("Reading settings \n");
  readSettings();
  digitalWrite(LED_PIN,HIGH); //disable LED that is enabled by default
  if(enableRelayOnPowerUp){
    enableRelay();
  }
  configTime("CET-1CEST,M3.5.0,M10.5.0/3","pl.pool.ntp.org");
  settimeofday_cb(timeSetCallback);
  debug_print("Initializing Wifi \n");
  initWifi();
  debug_print("Initializing webserver \n");
  initServer();
  debug_print("Setting interrupts \n");
  attachInterrupt(digitalPinToInterrupt(CF1_PIN), cf1IRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(CF_PIN), cfIRQ, FALLING);
  debug_print("Everything set up, proceeding to loop()\n");
  //delay(2000);
}

void loop() {
  //Section responsible for reading input of physical button
  #ifndef IGNORE_PHYSICAL_BUTTON
  bool buttonState = digitalRead(BUTTON_PIN);
  if(buttonState != 1 && millis()-lastTimeButtonStateChanged >= 400){
    relayStatus ? disableRelay() : enableRelay();
    lastTimeButtonStateChanged = millis();
  }
  #endif

  server.handleClient();

  if(WiFi.status() == WL_NO_SSID_AVAIL) initWifi(); //FIXME: Program execution will halt if WiFi signal is lost

  //collect statistics
  if(millis()-lastTimeStatCollected >= (statCollectingFrequency*1000) && statCollectingFrequency != 0 && statSendingFrequency != 0){ 
    lastTimeStatCollected = millis();
    stats->collectStat();
  }

  //collect energy data
  if(millis()-lastTimeEnergySent >= (energySendingFrequency*60000) && energySendingFrequency != 0){ 
    sendEnergyData(meter.getEnergyMeasurement());
    lastTimeEnergySent = millis();
  }
  //blink LED light
  if(millis() - lastTimeLEDBlink >= LED_BLINK_EVERY && blinkPhase == false && LEDmode == 2 && relayStatus){
    digitalWrite(LED_PIN,LOW);
    blinkPhase = !blinkPhase;
    lastTimeLEDBlink = millis();
  }
  if(millis() - lastTimeLEDBlink >= LED_BLINK_FOR && blinkPhase == true && LEDmode == 2){
    digitalWrite(LED_PIN,HIGH);
    blinkPhase = !blinkPhase;
    lastTimeLEDBlink = millis();
  }
}
