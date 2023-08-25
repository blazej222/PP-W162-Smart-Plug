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

void CollectedStats::collectStat(PowerMeter meter){
    Serial.println("Collecting stats");
    Serial.println(iterator);
    Serial.println(size);
    time[iterator] = timeClient.getEpochTime();
    if(measureVoltage)voltage[iterator] = meter.getVoltage();
    else voltage[iterator] = 0;
    if(measureCurrent)current[iterator] = meter.getCurrent();
    else current[iterator] = 0;
    power[iterator] = meter.getActivePower();
    if(iterator == size-1){

    }
    iterator++;
    if(iterator >= size) sendStatistics();
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
    Serial.println("Sending statistics");
    if(!client.connect(dataCollectingServerIP,dataCollectingServerPort)) return;
    if(client.print(this->serialize()) != 0)
    {
        this->zeroStatus();
        client.stop();
    }

}

void sendEnergyData(float x){
    Serial.println("Entered sendEnergyData");
    Serial.println(client.connect(dataCollectingServerIP,dataCollectingServerPort));
    String tmp = deviceName;
    tmp += "E:";
    tmp += String(x);
    Serial.println(client.print(tmp));
    client.stop();
}