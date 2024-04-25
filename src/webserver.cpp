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

bool debugEnabled = false;

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
    if(debugEnabled)tmp.replace("_DEBUGBOX_","checked");
    tmp.replace("_ENERGYTIME_",String(energySendingFrequency));
    tmp.replace("_STATTIME_",String(statCollectingFrequency));
    tmp.replace("_STATSEND_",String(statSendingFrequency));
    tmp.replace("_IP_",dataCollectingServerIP.toString());
    tmp.replace("_PORT_",String(dataCollectingServerPort));
    if(LEDmode==0) tmp.replace("_selected0","selected");
    else if(LEDmode==1) tmp.replace("_selected1","selected");
    else if(LEDmode==2) tmp.replace("_selected2","selected");
    tmp.replace("_VLTMULT_",String(meter.getVoltageMultiplier()));
    tmp.replace("_CRTMULT_",String(meter.getCurrentMultiplier()));
    tmp.replace("_PWRMULT_",String(meter.getPowerMultiplier()));
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
    if(server.arg("debugbox") == "on"){
        //if debug was disabled and is enabled now
        if(!debugEnabled){ 
            //Re-read mainpage
            File tmp = LittleFS.open("/main_debug.html","r");
            mainpage = tmp.readString();
            tmp.close();
        }
        debugEnabled = true;
    } 
    else{
        //if debug was enabled and is disabled now
        if(debugEnabled){
            //Re-read mainpage
            File tmp = LittleFS.open("/main.html","r");
            mainpage = tmp.readString();
            tmp.close();
        }
        debugEnabled = false;
    } 
    deviceName = server.arg("name");
    meter.setSwapWait(server.arg("DELAY").toInt());
    energySendingFrequency = server.arg("ENERGYTIME").toInt();
    statCollectingFrequency = server.arg("STATTIME").toInt();
    unsigned int tmpx = server.arg("STATSEND").toInt();
    if(statSendingFrequency != tmpx){
        if(stats != nullptr){
            stats->forceSendStatistics(); //try to send stats before overwriting collected data.
            delete stats;
        } 
        statSendingFrequency = tmpx;
        stats = new CollectedStats(statSendingFrequency);
    }
    dataCollectingServerIP.fromString(server.arg("IP"));
    dataCollectingServerPort = server.arg("PORT").toInt();
    LEDmode = server.arg("LEDmode").toInt();
    if(relayStatus && LEDmode == 1) digitalWrite(LED_PIN,LOW);
    else if(!relayStatus && LEDmode == 1) digitalWrite(LED_PIN,HIGH);
    else if(LEDmode == 2 || LEDmode == 0) digitalWrite(LED_PIN,HIGH);
    generateNewSettings();
    server.send(200,"text/html",submitpage);
}

void handle_root() {
    debug_print("Entered handle_root()\n");
    String tosend = mainpage;
    tosend.replace("_RATE_", String(mainWebsiteRefreshRate));
    //---------------------------------
    tosend.replace("_POWER_",String(meter.getActivePower()));
    tosend.replace("_PWRPULSE_",String(meter.getPowerPulse()));
    if(measureCurrent)
    {
        tosend.replace("_CURRENT_",String(meter.getCurrent()));
        if(debugEnabled) tosend.replace("_CPULSE_",String(meter.getCurrentPulse())); 
    }
    if(measureVoltage)
    {
        tosend.replace("_VOLTAGE_",String(meter.getVoltage()));
        if(debugEnabled) tosend.replace("_VPULSE_",String(meter.getVoltagePulse()));
    }
    if(debugEnabled) tosend.replace("_PPULSE_",String(meter.getPulseCount()));
    tosend.replace("_ENERGY_",String(meter.getEnergyMeasurement()));
    if(measureCurrent && measureVoltage) meter.swapCfMode(); //swap mode to current already so we might be able to skip busywait on current request
    relayStatus ? tosend.replace("_RELAY_","Enabled") : tosend.replace("_RELAY_","Disabled");
    tosend.replace("_NAME_",deviceName);
    server.send(200, "text/html", tosend);
    delay(100);
}

void handle_reset(){
    meter.resetEnergyMeasurement();
    server.send(200,"text/html",submitpage);
}

void handle_set_multipliers(){
    unsigned int voltageMultiplier = server.arg("voltagemult").toInt();
    unsigned int currentMultiplier = server.arg("currentmult").toInt();
    unsigned int powerMultiplier = server.arg("powermult").toInt();
    meter.setMultipliers(voltageMultiplier,powerMultiplier,currentMultiplier);
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
    server.on("/setmultipliers",HTTP_POST,handle_set_multipliers);
    server.on("/submit", HTTP_POST, handle_settings_submit);
    server.on("/reset",handle_reset);
    //server.onNotFound(handleNotFound);
    updater.setup(&server);
    server.begin();
}