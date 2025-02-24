#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "powerMeter.h"
class CollectedStats{
    time_t* timestamp;
    float* voltage;
    float* power;
    float* current;
    int iterator;
    int size;
    void sendStatistics();
    String serialize();
    void zeroStatus();
    public:
    CollectedStats(int size);
    ~CollectedStats();
    void collectStat();
    void forceSendStatistics();
};

extern unsigned int statCollectingFrequency; //collect statistics every seconds
extern unsigned int energySendingFrequency; //send energy usage every minutes
extern unsigned int statSendingFrequency; //send stats every x probes
extern IPAddress dataCollectingServerIP;
extern WiFiClient client;
extern uint16_t dataCollectingServerPort;
extern CollectedStats* stats;
extern String deviceName;

void sendEnergyData(float x);

extern bool measureCurrent;
extern bool measureVoltage;
extern PowerMeter meter;

