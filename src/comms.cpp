#include "comms.h"
#include "powerMeter.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

unsigned int statTime = 0; //collect statistics every seconds
unsigned int energySendingFrequency = 0; //send energy usage every minutes
unsigned int statSendingFrequency = 0; //send stats every x probes
IPAddress serveraddr;
WiFiClient client;
uint16_t port;
CollectedStats* stats = nullptr;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"pool.ntp.org",0,86400000);

CollectedStats::CollectedStats(int size){
    time = new unsigned long[size+20];
    voltage = new float[size+20];
    current = new float[size+20];
    power = new float[size+20];
    _size = size;
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
    for(int i = 0;i<_size+20;i++){
        time[i] = 0;
        voltage[i] = 0.;
        current[i] = 0.;
        power[i] = 0.;
    }
}

void CollectedStats::collectStat(PowerMeter meter){
    Serial.println("Collecting stats");
    Serial.println(iterator);
    Serial.println(_size);
    time[iterator] = timeClient.getEpochTime();
    if(measureVoltage)voltage[iterator] = meter.getVoltage();
    else voltage[iterator] = 0;
    if(measureCurrent)current[iterator] = meter.getCurrent();
    else current[iterator] = 0;
    power[iterator] = meter.getActivePower();
    if(iterator == _size-1){

    }
    iterator++;
    if(iterator >= _size) sendStatistics();
}

String CollectedStats::serialize(){
    String tmp = devicename;
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
    if(!client.connect(serveraddr,port)) return;
    if(client.print(this->serialize()) != 0)
    {
        this->zeroStatus();
        client.stop();
    }

}

void sendEnergyData(float x){
    Serial.println("Entered sendEnergyData");
    Serial.println(client.connect(serveraddr,port));
    String tmp = devicename;
    tmp += "E:";
    tmp += String(x);
    Serial.println(client.print(tmp));
    client.stop();
}