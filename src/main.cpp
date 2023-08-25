#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "defines.h"
#include "powerMeter.h"
#include "webserver.h"
#include "comms.h"
#include <LittleFS.h>

extern ESP8266WebServer server;

PowerMeter meter(RELAY_PIN,SEL_PIN,CF1_PIN,CF_PIN);

// Wifi settings
String SSID;
String KEY;
String devicename;

extern uint8_t rate; //page refresh rate
unsigned long last_time = 0;
unsigned long button_time = 0;
bool autoenable = false;
bool measureVoltage = true;
bool measureCurrent = false;

void IRAM_ATTR cf1irq(){
  meter.cf1_interrupt();
}

void IRAM_ATTR cfirq(){
  meter.cf_interrupt();
}

void enableRelay(){
    Serial.println("Enabling!");
    relaystatus = true;
    digitalWrite(RELAY_PIN,HIGH);
    digitalWrite(LIGHT_PIN,LOW);
}

void disableRelay(){
    Serial.println("Disabling!");
    relaystatus = false;
    digitalWrite(RELAY_PIN,LOW);
    digitalWrite(LIGHT_PIN,HIGH);
}

void initWifi(){
  WiFi.begin(SSID,KEY);
  unsigned long t1 = millis();
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
    if(millis()-t1 > 10000){
      Serial.println("Entered if");
      WiFi.disconnect();
      IPAddress local_IP(192,168,1,1);
      IPAddress gateway(192,168,1,1);
      IPAddress subnet(255,255,255,0);
      WiFi.softAPConfig(local_IP, gateway, subnet);
      WiFi.softAP("Energy Meter","root123root123");
      break;
    }
  }; // hang until connection can be made
  Serial.println("Wifi initialized");

}

void readSettings(){
  if(!LittleFS.begin())Serial.println("LittleFS intialization failure");
  delay(1000);
  File settings = LittleFS.open("/config.cfg","r");
  unsigned int crt = settings.readStringUntil('\n').toInt();
  unsigned int vlt = settings.readStringUntil('\n').toInt();
  unsigned int pwr = settings.readStringUntil('\n').toInt();
  meter.setMultipliers(vlt,pwr,crt);
  rate = settings.readStringUntil('\n').toInt();
  SSID = settings.readStringUntil('\n');
  KEY = settings.readStringUntil('\n');
  autoenable = settings.readStringUntil('\n').toInt();
  devicename = settings.readStringUntil('\n');
  measureVoltage = settings.readStringUntil('\n').toInt();
  measureCurrent = settings.readStringUntil('\n').toInt();
  meter.swapWait = settings.readStringUntil('\n').toInt();
  energySendingFrequency = settings.readStringUntil('\n').toInt();
  statTime = settings.readStringUntil('\n').toInt();
  statSendingFrequency = settings.readStringUntil('\n').toInt();
  if(statSendingFrequency != 0) stats = new CollectedStats(statSendingFrequency);
  serveraddr.fromString(settings.readStringUntil('\n'));
  port = settings.readStringUntil('\n').toInt();
  settings.close();
}

void generateNewSettings(){
  String newsettings = String(meter.getCurrentMultiplier());
  newsettings += '\n';
  newsettings += String(meter.getVoltageMultiplier());
  newsettings += '\n';
  newsettings += String(meter.getPowerMultiplier());
  newsettings += '\n';
  newsettings += String(rate);
  newsettings += '\n';
  newsettings += SSID;
  newsettings += '\n';
  newsettings += KEY;
  newsettings += '\n';
  newsettings += autoenable;
  newsettings += '\n';
  newsettings += devicename;
  newsettings += '\n';
  newsettings += measureVoltage;
  newsettings += '\n';
  newsettings += measureCurrent;
  newsettings += '\n';
  newsettings += meter.swapWait;
  newsettings += '\n';
  newsettings += energySendingFrequency;
  newsettings += '\n';
  newsettings += statTime;
  newsettings += '\n';
  newsettings += statSendingFrequency;
  newsettings += '\n';
  newsettings += serveraddr.toString();
  newsettings += '\n';
  newsettings += port;
  File settings = LittleFS.open("/config.cfg","w");
  settings.write(newsettings.c_str());
  settings.close();
}

void setup() {
  pinMode(RELAY_PIN,OUTPUT);
  pinMode(LIGHT_PIN,OUTPUT);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  // Serial.begin(115200);
  Serial.println("Reading settings");
  readSettings();
  if(autoenable){
    enableRelay();
  }
  Serial.println("Initializing Wifi...");
  initWifi();
  Serial.println("Initializing webserver");
  initServer();
  Serial.println("Setting interrupts...");
  attachInterrupt(digitalPinToInterrupt(CF1_PIN), cf1irq, FALLING);
  attachInterrupt(digitalPinToInterrupt(CF_PIN), cfirq, FALLING);
  delay(2000);
  Serial.println("WifiAddr is:");
  Serial.print(WiFi.localIP());
}

void loop() {
  bool button_state = digitalRead(BUTTON_PIN);
  if(button_state != 1 && millis()-button_time >= 400){
    relaystatus ? disableRelay() : enableRelay();
    button_time = millis();
  }
  timeClient.update();
  server.handleClient();
  if(WiFi.status() == WL_NO_SSID_AVAIL) initWifi();
  if(millis()-last_time >= (statTime*1000) && statTime != 0 && statSendingFrequency != 0){
    last_time = millis();
    stats->collectStat(meter);
  }
  if(millis()-last_time >= (energySendingFrequency*60000) && energySendingFrequency != 0){
    sendEnergyData(meter.getEnergyMeasurement());
    last_time = millis();
  }
}
