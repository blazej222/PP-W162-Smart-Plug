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
bool enableRelayOnPowerUp = false;
bool measureVoltage = true;
bool measureCurrent = false;

void IRAM_ATTR cf1IRQ(){
  meter.cf1Interrupt();
}

void IRAM_ATTR cfIRQ(){
  meter.cfInterrupt();
}

void enableRelay(){
    Serial.println("Enabling!");
    relayStatus = true;
    digitalWrite(RELAY_PIN,HIGH);
    digitalWrite(LED_PIN,LOW);
}

void disableRelay(){
    Serial.println("Disabling!");
    relayStatus = false;
    digitalWrite(RELAY_PIN,LOW);
    digitalWrite(LED_PIN,HIGH);
}

void initWifi(){
  WiFi.begin(SSID,KEY);
  unsigned long t1 = millis();
  while (WiFi.status() != WL_CONNECTED){
    if(millis()-t1 > WIFI_TIMEOUT){ //if connection cannot be made within reasonable time
      WiFi.disconnect();
      IPAddress local_IP(192,168,1,1);
      IPAddress gateway(192,168,1,1);
      IPAddress subnet(255,255,255,0);
      WiFi.softAPConfig(local_IP, gateway, subnet);
      WiFi.softAP("Energy Meter","root123root123"); //start own AP
      break;
    }
  }; // hang until connection can be made
  Serial.println("Wifi initialized");

}

void readSettings(){
  if(!LittleFS.begin())Serial.println("LittleFS intialization failure");
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
  meter.swapWait = settings.readStringUntil('\n').toInt();
  energySendingFrequency = settings.readStringUntil('\n').toInt();
  statCollectingFrequency = settings.readStringUntil('\n').toInt();
  statSendingFrequency = settings.readStringUntil('\n').toInt();
  if(statSendingFrequency != 0) stats = new CollectedStats(statSendingFrequency);
  dataCollectingServerIP.fromString(settings.readStringUntil('\n'));
  dataCollectingServerPort = settings.readStringUntil('\n').toInt();
  settings.close();
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
  newSettings += meter.swapWait;
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
  File settings = LittleFS.open("/config.cfg","w");
  settings.write(newSettings.c_str());
  settings.close();
}

void setup() {
  pinMode(RELAY_PIN,OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  // Serial.begin(115200);
  Serial.println("Reading settings");
  readSettings();
  if(enableRelayOnPowerUp){
    enableRelay();
  }
  Serial.println("Initializing Wifi...");
  initWifi();
  Serial.println("Initializing webserver");
  initServer();
  Serial.println("Setting interrupts...");
  attachInterrupt(digitalPinToInterrupt(CF1_PIN), cf1IRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(CF_PIN), cfIRQ, FALLING);
  //delay(2000);
  Serial.println("WifiAddr is:");
  Serial.print(WiFi.localIP());
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);
  if(buttonState != 1 && millis()-lastTimeButtonStateChanged >= 400){
    relayStatus ? disableRelay() : enableRelay();
    lastTimeButtonStateChanged = millis();
  }
  timeClient.update();
  server.handleClient();
  if(WiFi.status() == WL_NO_SSID_AVAIL) initWifi(); //FIXME: Program execution will halt if WiFi signal is lost
  if(millis()-lastTimeStatCollected >= (statCollectingFrequency*1000) && statCollectingFrequency != 0 && statSendingFrequency != 0){
    lastTimeStatCollected = millis();
    stats->collectStat(meter);
  }
  if(millis()-lastTimeEnergySent >= (energySendingFrequency*60000) && energySendingFrequency != 0){
    sendEnergyData(meter.getEnergyMeasurement());
    lastTimeEnergySent = millis();
  }
}
