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

/**
 * 
 * @return Active power usage value in watts
 */
float PowerMeter::getActivePower(){
  if ((micros() - lastCfInterruptTimestamp) > METERING_TIMEOUT) powerPulseLength = 0;             //if no changes received for some time
  unsigned long localPowerPulseLength = powerPulseLength;                                         //make sure an interrupt does not change any values mid processing
  lastPowerPulseLength = localPowerPulseLength;
  return (localPowerPulseLength > 0) ? (float)powerMultiplier / (float)localPowerPulseLength : 0; // return power usage
}


/**
 * 
 * @return Voltage value in volts
 */
float PowerMeter::getVoltage(){
  if(!meterMode){
    swapCfMode();
  }
  if(pulsesReceived != 2){
    unsigned long timestamp = millis();
    while(pulsesReceived != 2 && millis()-timestamp < METER_MODESWAP_TIMEOUT){  //busy wait until we get frequency or timeout is reached
      yield();
    } 
  }
  if ((micros() - lastCf1InterruptTimestamp) > METERING_TIMEOUT || pulsesReceived != 2) voltagePulseLength = 0;     
  unsigned long localVoltagePulseLength = voltagePulseLength;
  lastVoltagePulseLength = localVoltagePulseLength;
   return (localVoltagePulseLength > 0) ? (float)voltageMultiplier / (float)localVoltagePulseLength : 0;
}

/**
 * 
 * @return Current value in amperes
 */
float PowerMeter::getCurrent(){
  if(meterMode){  //if meter is currently in measuring voltage mode we need to switch it to measuring current mode
    swapCfMode();
  }
  if(pulsesReceived != 2){
    unsigned long timestamp = millis();
    while(pulsesReceived != 2 && millis()-timestamp < METER_MODESWAP_TIMEOUT){     //busy wait until we get frequency or timeout is reached
      yield();
    } 
  }
  if ((micros() - lastCf1InterruptTimestamp) > METERING_TIMEOUT || pulsesReceived != 2) currentPulseLength = 0;
  unsigned long localCurrentPulseLength = currentPulseLength;
  lastCurrentPulseLength = localCurrentPulseLength;
   return (localCurrentPulseLength > 0) ? (float)currentMultiplier / (float)localCurrentPulseLength : 0;
}

/**
 * @return raw power pulse length registered when last calling getActivePower()
 */
unsigned long PowerMeter::getPowerPulse(){
  return lastPowerPulseLength;
}

/**
 * @return raw current pulse length registered when last calling getCurrent() 
 */
unsigned long PowerMeter::getCurrentPulse(){
  return lastCurrentPulseLength;
}

/**
 * @return raw voltage pulse length registered when last calling getVoltage()
 */
unsigned long PowerMeter::getVoltagePulse(){
  return lastVoltagePulseLength;
}

void IRAM_ATTR PowerMeter::cf1Interrupt() {
    unsigned long now = micros();
    //if true we are measuring voltage
    if(meterMode) voltagePulseLength = now - lastCf1InterruptTimestamp;
    //if false we are measuring current
    else currentPulseLength = now - lastCf1InterruptTimestamp;
    lastCf1InterruptTimestamp = now;
    //increment pulses counter
    if(pulsesReceived < 2) pulsesReceived++;
}
void IRAM_ATTR PowerMeter::cfInterrupt() {
    unsigned long now = micros();
    powerPulseLength = now - lastCfInterruptTimestamp;
    lastCfInterruptTimestamp = now;
    pulseCount++;
}

void PowerMeter::swapCfMode(){
  meterMode = !meterMode;
  digitalWrite(selPin,meterMode);
  delayMicroseconds(swapWait);
  pulsesReceived = 0;
  //lastCfInterruptTimestamp = micros();
}

void PowerMeter::calibrateVoltage(float expected){
  float vlt = getVoltage();
  if(vlt != 0){
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

unsigned int PowerMeter::getSwapWait(){
  return swapWait;
}

void PowerMeter::setSwapWait(unsigned int x){
  swapWait = x;
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