#pragma once
#include "defines.h"
class PowerMeter
{
    //PINS
    const uint8_t relay;
    const uint8_t sel;
    const uint8_t cf1;
    const uint8_t cf;
    //Variables used with interrupt routines
    volatile unsigned long power_pulse_width = 0;
    volatile unsigned long voltage_pulse_width = 0;
    volatile unsigned long current_pulse_width = 0;
    volatile unsigned long last_cf_interrupt = 0;
    volatile unsigned long last_cf1_interrupt = 0;
    volatile unsigned long long pulse_count = 0;

    bool voltageMode = false;

    unsigned int voltage_multiplier = VOLTAGE_RATIO;
    unsigned int power_multiplier = POWER_RATIO;
    unsigned int current_multiplier = CURRENT_RATIO;

    void setup();
    void swapCfMode();
    public:
        unsigned int swapWait  = 500; //amount of miliseconds to wait before switching current/voltage //TODO:Create proper getters/setters
        PowerMeter(uint8_t _relay,uint8_t _sel,uint8_t _cf1,uint8_t _cf);
        float getActivePower();
        float getVoltage();
        float getCurrent();
        unsigned long getPowerPulse();
        unsigned long getVoltagePulse();
        unsigned long getCurrentPulse();
        unsigned int getVoltageMultiplier();
        unsigned int getCurrentMultiplier();
        unsigned int getPowerMultiplier();
        void IRAM_ATTR cf1_interrupt();
        void IRAM_ATTR cf_interrupt();
        unsigned long long getPulseCount();
        void calibrateVoltage(float expected);
        void calibrateCurrent(float expected);
        void calibratePower(float expected);
        void setMultipliers(unsigned int voltage,unsigned int power,unsigned int current);
        float getEnergyMeasurement();
        void resetEnergyMeasurement();
};