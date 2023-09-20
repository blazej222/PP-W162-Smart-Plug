#pragma once
#include "defines.h"
class PowerMeter
{
    //PINS
    const uint8_t relayPin;
    const uint8_t selPin;
    const uint8_t cf1Pin;
    const uint8_t cfPin;
    //Variables used with interrupt routines
    volatile unsigned long powerPulseLength = 0;
    volatile unsigned long voltagePulseLength = 0;
    volatile unsigned long currentPulseLength = 0;
    volatile unsigned long lastCfInterruptTimestamp = 0;
    volatile unsigned long lastCf1InterruptTimestamp = 0;
    volatile unsigned long long pulseCount = 0;

    bool voltageMode = false;

    unsigned int voltageMultiplier = VOLTAGE_RATIO;
    unsigned int powerMultiplier = POWER_RATIO;
    unsigned int currentMultiplier = CURRENT_RATIO;
    unsigned int swapWait = 500; //amount of miliseconds to wait before switching current/voltage
    
    void setup();
    void swapCfMode();
    public:
        PowerMeter(uint8_t _relay,uint8_t _sel,uint8_t _cf1,uint8_t _cf);
        float getActivePower();
        float getVoltage();
        float getCurrent();
        unsigned int getSwapWait();
        void setSwapWait(unsigned int x);
        unsigned long getPowerPulse();
        unsigned long getVoltagePulse();
        unsigned long getCurrentPulse();
        unsigned int getVoltageMultiplier();
        unsigned int getCurrentMultiplier();
        unsigned int getPowerMultiplier();
        unsigned long long getPulseCount();
        float getEnergyMeasurement();
        void IRAM_ATTR cf1Interrupt();
        void IRAM_ATTR cfInterrupt();
        void calibrateVoltage(float expected);
        void calibrateCurrent(float expected);
        void calibratePower(float expected);
        void setMultipliers(unsigned int voltage,unsigned int power,unsigned int current);
        void resetEnergyMeasurement();
};