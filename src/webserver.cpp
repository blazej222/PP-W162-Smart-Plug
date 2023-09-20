#include <Arduino.h>
#include <LittleFS.h>
#include "webserver.h"
#include "defines.h"
#include "comms.h"

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updater;

String mainpage;
String settingspage;
String submitpage;

bool relayStatus = false;

uint8_t mainWebsiteRefreshRate;

void handle_enable(){
    enableRelay();
    server.send(200, "text/html", submitpage);
}

void handle_disable(){
    disableRelay();
    server.send(200, "text/html", submitpage);
}

void handle_settings(){
    String tmp = settingspage;
    tmp.replace("_RATE_",String(mainWebsiteRefreshRate));
    tmp.replace("_SSID_",SSID);
    tmp.replace("_PASSWORD_",KEY);
    tmp.replace("_NAME_",deviceName);
    tmp.replace("_DELAY_",String(meter.getSwapWait()));
    if(enableRelayOnPowerUp)tmp.replace("_CHCK_","checked");
    if(measureVoltage)tmp.replace("_VOLTAGEBOX_","checked");
    if(measureCurrent)tmp.replace("_CURRENTBOX_","checked");
    tmp.replace("_ENERGYTIME_",String(energySendingFrequency));
    tmp.replace("_STATTIME_",String(statCollectingFrequency));
    tmp.replace("_STATSEND_",String(statSendingFrequency));
    tmp.replace("_IP_",dataCollectingServerIP.toString());
    tmp.replace("_PORT_",String(dataCollectingServerPort));
    server.send(200, "text/html", tmp);
}

void handle_calibrate_submit(){
    float vlt = server.arg("voltage").toFloat();
    float crt = server.arg("current").toFloat();
    float pwr = server.arg("power").toFloat();
    if(vlt != 0.0) meter.calibrateVoltage(vlt);
    if(crt != 0.0) meter.calibrateCurrent(crt);
    if(pwr != 0.0) meter.calibratePower(pwr);
    generateNewSettings();
    server.send(200,"text/html",submitpage);
}

void handle_settings_submit(){
    uint8_t tmp = server.arg("rrate").toInt();
    if(tmp > 0 && tmp < 1000) mainWebsiteRefreshRate = tmp;
    String temporary = server.arg("SSID");
    if(!temporary.equals("")) SSID = temporary;
    temporary = server.arg("PWD");
    if(!temporary.equals("")) KEY = temporary;
    if(server.arg("box") == "on") enableRelayOnPowerUp = true;
    else enableRelayOnPowerUp = false;
    if(server.arg("voltagebox") == "on") measureVoltage = true;
    else measureVoltage = false;
    if(server.arg("currentbox") == "on") measureCurrent = true;
    else measureCurrent = false;
    deviceName = server.arg("name");
    meter.setSwapWait(server.arg("DELAY").toInt());
    energySendingFrequency = server.arg("ENERGYTIME").toInt();
    statCollectingFrequency = server.arg("STATTIME").toInt();
    unsigned int tmpx = server.arg("STATSEND").toInt();
    if(statSendingFrequency != tmpx){
        if(stats != nullptr) delete stats;
        statSendingFrequency = tmpx;
        stats = new CollectedStats(statSendingFrequency);
    }
    dataCollectingServerIP.fromString(server.arg("IP"));
    dataCollectingServerPort = server.arg("PORT").toInt();
    generateNewSettings();
    server.send(200,"text/html",submitpage);
}

void handle_root() {
    debug_print("Entered handle_root()\n");
    String tosend = mainpage;
    tosend.replace("_RATE_", String(mainWebsiteRefreshRate));
    //---------------------------------
    tosend.replace("_POWER_",String(meter.getActivePower()));
    if(measureVoltage)
    {
        tosend.replace("_VPULSE_",String(meter.getVoltagePulse())); //we keep it near so it executes in the same phase of cf cycle
        tosend.replace("_VOLTAGE_",String(meter.getVoltage()));
    }
    if(measureCurrent)
    {
        tosend.replace("_CPULSE_",String(meter.getCurrentPulse())); 
        tosend.replace("_CURRENT_",String(meter.getCurrent())); 
    }
    tosend.replace("_PWRPULSE_",String(meter.getPowerPulse()));
    tosend.replace("_PMUL_",String(meter.getPowerMultiplier()));
    tosend.replace("_CMUL_",String(meter.getCurrentMultiplier()));
    tosend.replace("_VMUL_",String(meter.getVoltageMultiplier()));
    tosend.replace("_ENERGY_",String(meter.getEnergyMeasurement()));
    tosend.replace("_PPULSE_",String(meter.getPulseCount()));
    relayStatus ? tosend.replace("_RELAY_","Enabled") : tosend.replace("_RELAY_","Disabled");
    tosend.replace("_NAME_",deviceName);
    server.send(200, "text/html", tosend);
    delay(100);
}

void handle_reset(){
    meter.resetEnergyMeasurement();
    server.send(200,"text/html",submitpage);
}

void initServer(){
    //handling filesystem
    // LittleFS initialization routine
    File mainfile = LittleFS.open("/main.html","r");
    mainpage = mainfile.readString();
    mainfile.close();
    File settingsfile = LittleFS.open("/settings.html","r");
    settingspage = settingsfile.readString();
    settingsfile.close();
    File submitfile = LittleFS.open("/submit.html","r");
    submitpage = submitfile.readString();
    submitfile.close();
    //-------
    server.on("/", handle_root);
    server.on("/enable", handle_enable);
    server.on("/disable", handle_disable);
    server.on("/settings",handle_settings);
    server.on("/generate_204", handle_root);
    server.on("/calibrate", HTTP_POST, handle_calibrate_submit);
    server.on("/submit", HTTP_POST, handle_settings_submit);
    server.on("/reset",handle_reset);
    //server.onNotFound(handleNotFound);
    updater.setup(&server);
    server.begin();
}