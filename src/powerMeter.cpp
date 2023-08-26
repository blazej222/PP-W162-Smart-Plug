#include <Arduino.h>
#include "defines.h"
#include "powerMeter.h"

PowerMeter::PowerMeter(uint8_t _relay,uint8_t _sel,uint8_t _cf1,uint8_t _cf) : relayPin(_relay),selPin(_sel),cf1Pin(_cf1),cfPin(_cf){
  this->setup();
  return;
};

void PowerMeter::setup(){
    pinMode(selPin,OUTPUT);
    pinMode(cf1Pin,INPUT);
    pinMode(cfPin,INPUT);
    digitalWrite(selPin,LOW); //Measure current instead of voltage

}

float PowerMeter::getActivePower(){
  if ((micros() - lastCfInterruptTimestamp) > METERING_TIMEOUT) powerPulseLength = 0; //if no changes received for some time
  return (powerPulseLength > 0) ? (float)powerMultiplier / (float)powerPulseLength : 0; // return power usage
}

float PowerMeter::getVoltage(){
  if ((micros() - lastCf1InterruptTimestamp) > METERING_TIMEOUT) voltagePulseLength = 0;
  if(!voltageMode){
    swapCfMode();
  }
   return (voltagePulseLength > 0) ? (float)voltageMultiplier / (float)voltagePulseLength : 0;
}

float PowerMeter::getCurrent(){
  if ((micros() - lastCf1InterruptTimestamp) > METERING_TIMEOUT) currentPulseLength = 0;
  if(voltageMode){
    swapCfMode();
  }
   return (currentPulseLength > 0) ? (float)currentMultiplier / (float)currentPulseLength : 0;
}

unsigned long PowerMeter::getPowerPulse(){
  return powerPulseLength;
}

unsigned long PowerMeter::getCurrentPulse(){
  if(voltageMode){
    swapCfMode();
  }
  return currentPulseLength;
}

unsigned long PowerMeter::getVoltagePulse(){
  if(!voltageMode){
    swapCfMode();
  }
  return voltagePulseLength;
}

void IRAM_ATTR PowerMeter::cf1Interrupt() {
    unsigned long now = micros();
    if(voltageMode) voltagePulseLength = now - lastCf1InterruptTimestamp;
    else currentPulseLength = now - lastCf1InterruptTimestamp;
    lastCf1InterruptTimestamp = now;
}
void IRAM_ATTR PowerMeter::cfInterrupt() {
    unsigned long now = micros();
    powerPulseLength = now - lastCfInterruptTimestamp;
    lastCfInterruptTimestamp = now;
    pulseCount++;
}

void PowerMeter::swapCfMode(){
  voltageMode = !voltageMode;
  digitalWrite(selPin,voltageMode);
  delay(swapWait);
  lastCfInterruptTimestamp = micros();
}

void PowerMeter::calibrateVoltage(float expected){
  float vlt = getVoltage();
  if(vlt != 0){
  Serial.println(expected);
  Serial.println(vlt);
  Serial.println(expected/vlt);
  voltageMultiplier = (float)voltageMultiplier * (expected/vlt);
  }
}

void PowerMeter::calibrateCurrent(float expected){
  if(getCurrent() != 0)
  currentMultiplier = (float)currentMultiplier * (expected/getCurrent());
}

void PowerMeter::calibratePower(float expected){
  if(getActivePower() != 0)
  powerMultiplier = (float)powerMultiplier * (expected/getActivePower());
}

unsigned int PowerMeter::getPowerMultiplier(){
  return powerMultiplier;
}

unsigned int PowerMeter::getCurrentMultiplier(){
  return currentMultiplier;
}

unsigned int PowerMeter::getVoltageMultiplier(){
  return voltageMultiplier;
}

float PowerMeter::getEnergyMeasurement(){
  return ((float)powerMultiplier / 1000000. / 3600.) * (float)pulseCount ;
}

void PowerMeter::resetEnergyMeasurement(){
  pulseCount = 0;
}

void PowerMeter::setMultipliers(unsigned int voltage,unsigned int power,unsigned int current){
  voltageMultiplier = voltage;
  powerMultiplier = power;
  currentMultiplier = current;
}

unsigned long long PowerMeter::getPulseCount(){
  return pulseCount;
}