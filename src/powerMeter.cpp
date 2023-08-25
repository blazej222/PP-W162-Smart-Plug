#include "defines.h"
#include <Arduino.h>
#include "powerMeter.h"

PowerMeter::PowerMeter(uint8_t _relay,uint8_t _sel,uint8_t _cf1,uint8_t _cf) : relay(_relay),sel(_sel),cf1(_cf1),cf(_cf){
  this->setup();
  return;
};

void PowerMeter::setup(){
    pinMode(sel,OUTPUT);
    pinMode(cf1,INPUT);
    pinMode(cf,INPUT);
    digitalWrite(sel,LOW); //Measure current instead of voltage

}

float PowerMeter::getActivePower(){
  if ((micros() - last_cf_interrupt) > 500000) power_pulse_width = 0; //if no changes received for some time
  return (power_pulse_width > 0) ? (float)power_multiplier / (float)power_pulse_width : 0; // return power usage
}

float PowerMeter::getVoltage(){
  if(!voltageMode){
    swapCfMode();
  }
   return (voltage_pulse_width > 0) ? (float)voltage_multiplier / (float)voltage_pulse_width : 0;
}

float PowerMeter::getCurrent(){
  if(voltageMode){
    swapCfMode();
  }
   return (current_pulse_width > 0) ? (float)current_multiplier / (float)current_pulse_width : 0;
}

unsigned long PowerMeter::getPowerPulse(){
  return power_pulse_width;
}

unsigned long PowerMeter::getCurrentPulse(){
  if(voltageMode){
    swapCfMode();
  }
  return current_pulse_width;
}

unsigned long PowerMeter::getVoltagePulse(){
  if(!voltageMode){
    swapCfMode();
  }
  return voltage_pulse_width;
}

void IRAM_ATTR PowerMeter::cf1_interrupt() {
    unsigned long now = micros();
    if(voltageMode) voltage_pulse_width = now - last_cf1_interrupt;
    else current_pulse_width = now - last_cf1_interrupt;
    last_cf1_interrupt = now;
}
void IRAM_ATTR PowerMeter::cf_interrupt() {
    unsigned long now = micros();
    power_pulse_width = now - last_cf_interrupt;
    last_cf_interrupt = now;
    pulse_count++;
}

void PowerMeter::swapCfMode(){
  voltageMode = !voltageMode;
  digitalWrite(sel,voltageMode);
  delay(swapWait);
  last_cf_interrupt = micros();
}

void PowerMeter::calibrateVoltage(float expected){
  float vlt = getVoltage();
  if(vlt != 0){
  Serial.println(expected);
  Serial.println(vlt);
  Serial.println(expected/vlt);
  voltage_multiplier = (float)voltage_multiplier * (expected/vlt);
  }
}

void PowerMeter::calibrateCurrent(float expected){
  if(getCurrent() != 0)
  current_multiplier = (float)current_multiplier * (expected/getCurrent());
}

void PowerMeter::calibratePower(float expected){
  if(getActivePower() != 0)
  power_multiplier = (float)power_multiplier * (expected/getActivePower());
}

unsigned int PowerMeter::getPowerMultiplier(){
  return power_multiplier;
}

unsigned int PowerMeter::getCurrentMultiplier(){
  return current_multiplier;
}

unsigned int PowerMeter::getVoltageMultiplier(){
  return voltage_multiplier;
}

float PowerMeter::getEnergyMeasurement(){
  return ((float)power_multiplier / 1000000. / 3600.) * (float)pulse_count ;
}

void PowerMeter::resetEnergyMeasurement(){
  pulse_count = 0;
}

void PowerMeter::setMultipliers(unsigned int voltage,unsigned int power,unsigned int current){
  voltage_multiplier = voltage;
  power_multiplier = power;
  current_multiplier = current;
}

unsigned long long PowerMeter::getPulseCount(){
  return pulse_count;
}