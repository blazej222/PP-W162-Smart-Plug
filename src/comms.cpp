#include "comms.h"
#include "powerMeter.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

unsigned int statCollectingFrequency = 0; //collect statistics every seconds
unsigned int energySendingFrequency = 0; //send energy usage every minutes
unsigned int statSendingFrequency = 0; //send stats every x probes
IPAddress dataCollectingServerIP;
WiFiClient client;
uint16_t dataCollectingServerPort;
CollectedStats* stats = nullptr;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"pool.ntp.org",0,86400000);

CollectedStats::CollectedStats(int _size){
    time = new unsigned long[_size+20];
    voltage = new float[_size+20];
    current = new float[_size+20];
    power = new float[_size+20];
    size = _size;
    this->zeroStatus();
}

CollectedStats::~CollectedStats(){
    delete[] time;
    delete[] voltage;
    delete[] current;
    delete[] power;
}

void CollectedStats::zeroStatus(){
    iterator = 0;
    for(int i = 0;i<size+20;i++){
        time[i] = 0;
        voltage[i] = 0.;
        current[i] = 0.;
        power[i] = 0.;
    }
}

void CollectedStats::forceSendStatistics(){
    if(iterator != 0) sendStatistics(); //if there's any data that can be sent, send it
}

void CollectedStats::collectStat(){
    debug_print("Collecting stats\n");
    debug_print("Max storage size:");
    debug_print(size);
    debug_print("\nCurrent storage size:");
    debug_print(iterator);
    debug_print("\n");
    time[iterator] = timeClient.getEpochTime();
    power[iterator] = meter.getActivePower();
    if(measureCurrent)current[iterator] = meter.getCurrent();
    else current[iterator] = 0;
    if(measureVoltage)voltage[iterator] = meter.getVoltage();
    else voltage[iterator] = 0;
    iterator++;
    if(measureCurrent && measureVoltage) meter.swapCfMode(); //swap mode to current already so we might be able to skip busywait on current request
    if(iterator >= size) sendStatistics();
    if(iterator >= size+20) this->zeroStatus();
}

String CollectedStats::serialize(){
    String tmp = deviceName;
    tmp += "T:";
    for(int i = 0;i<iterator;i++){
        tmp += time[i];
        tmp += ";";
    } 
    tmp += "V:";
    for(int i = 0;i<iterator;i++){
        tmp += voltage[i];
        tmp += ";";
    } 
    tmp += "C:";
    for(int i = 0;i<iterator;i++){
        tmp += current[i];
        tmp += ";";
    } 
    tmp += "P:";
    for(int i = 0;i<iterator;i++){
        tmp += power[i];
        tmp += ";";
    } 
    return tmp;
}

void CollectedStats::sendStatistics(){
    debug_print("Sending statistics to server:");
    debug_print(dataCollectingServerIP.toString());
    debug_print('\n');
    bool result = client.connect(dataCollectingServerIP,dataCollectingServerPort);
    if(result){
        debug_print("Succesfully connected.\n");
    }else{
        debug_print("Couldn't estabilish connection to server\n");
        return;
    }
    if(client.print(this->serialize()) != 0) //if we were able to transmit data
    {
        debug_print("Data has been succesfully sent, zeroing storage\n");
        this->zeroStatus();
    }
    client.stop();
}

void sendEnergyData(float x){
    debug_print("Attempting to send energy data to server:");
    debug_print(dataCollectingServerIP.toString());
    debug_print('\n');
    bool result = client.connect(dataCollectingServerIP,dataCollectingServerPort);
    if(result){
        debug_print("Succesfully connected.\n");
    }else{
        debug_print("Couldn't estabilish connection to server\n");
        return;
    }
    String tmp = deviceName;
    tmp += "E:";
    tmp += String(x);
    client.print(tmp);
    client.stop();
}