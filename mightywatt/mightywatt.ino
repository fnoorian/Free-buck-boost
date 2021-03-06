// MightyWatt firmware 2.3.2
// by: Jakub Polonský <http://kaktuscircuits.blogspot.cz/2015/03/mightywatt-resource-page.html>
// This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.

// IMPORTANT!
// UNCOMMENT THE LINE THAT BELONGS TO YOUR ARDUINO VERSION, COMMENT THE OTHER LINE
//#define ARM // Arduino Due and similar ATSAM (32bit) based boards via Programming port (USB)
//#define AVR // Arduino Uno and similar AVR (8bit) based boards // This should be automatically set by the IDE

// WARNING!
// TO MAKE THE MIGHTYWATT WORKING ON ARDUINO DUE, YOU MUST HAVE THE JUMPER ON DUE SET FOR EXTERNAL ANALOG REFERENCE
#include <Wire.h>
#ifdef AVR
  #define I2C Wire
#endif
#ifdef ARM
  #define I2C Wire1
#endif
#include <math.h>

// <Device Information>
#define FW_VERSION "2.3.2" // Universal for AVR and ARM
#define BOARD_REVISION "r2.3" // minimum MightyWatt board revision for this sketch is 2.3
#define DVM_INPUT_RESISTANCE 330000 // differential input resistance
// AD5691 DAC
// </Device Information>
#define SERIAL_BUFF_MAX 128

// <Pins>
const int REMOTE_PIN = 2;
const int CV_PIN = 3;
const int V_GAIN_PIN = 4;
const int ADC_I_PIN = A1;
const int ADC_V_PIN = A2;
const int ADC_T_PIN = A3;
// </Pins>

// <DAC>
//  commands
const byte DAC_ADDRESS = 0b1001100; // I2C address of DAC (A0 tied to GND)
const byte DAC_WRITE_DAC_AND_INPUT_REGISTERS = 0b00110000; // write to DAC memory 
//  value
unsigned int dac = 0;
//  calibration
const unsigned int IDAC_SLOPE = 11522 / 2;
const int IDAC_INTERCEPT = -17;
const unsigned int VDAC0_SLOPE = 31445;
const int VDAC0_INTERCEPT = -25;
const unsigned int VDAC1_SLOPE = 5426;
const int VDAC1_INTERCEPT = -22;
// </DAC>

// <Serial>
byte commandByte; // first byte of TX/RX data
const unsigned int SERIAL_TIMEOUT = 10000; // approx. 40 milliseconds for receiving all the data
// commands
const byte QDC = 30; // query device capabilities
const byte IDN = 31; // identification request
const byte MR = 0;   // Measurement report
// set values
unsigned int setCurrent = 0;
unsigned int setVoltage = 0;
unsigned long setPower = 0;
unsigned long setResistance = 0;
// </Serial>

// <Watchdog>
unsigned int watchdogCounter = 0;
const unsigned int WATCHDOG_TIMEOUT = 500;
byte isWatchdogEnabled = 0;
// </Watchdog>

// <Thermal Management>
const unsigned long MAX_POWER = 75000; // 75 Watts maximum power
const byte TEMPERATURE_THRESHOLD = 110;
byte temperature = 25;
//  thermistor values
const float THERMISTOR_B = 3455;
const float THERMISTOR_T0 = 298.15;
const float THERMISTOR_R0 = 10000;
const float THERMISTOR_R1 = 1000;
// </Thermal Management>

// <Voltmeter>
unsigned int voltage = 0;
byte vRange = 0; // 0 = gain 1, 1 = gain 5.7
//  hysteresis
unsigned int voltageRangeDown; // switch gain when going under
// calibration constants
const unsigned int VADC0_SLOPE = 32084;
const unsigned int VADC1_SLOPE = 5446;
const int VADC0_INTERCEPT = 140;
const int VADC1_INTERCEPT = -2;
// serial communication
byte remoteStatus = 0; // 0 = local, 1 = remote
const byte REMOTE_ID = 4;
// </Voltmeter>

// <Ammeter>
unsigned int current = 0;
//  calibration constants
const unsigned int IADC_SLOPE = 11400;
const char IADC_INTERCEPT = 5;
// </Ammeter>

// <Status>
const byte READY = 0;
// others stop device automatically
const byte CURRENT_OVERLOAD = 1; // both ADC and DAC
const byte VOLTAGE_OVERLOAD = 2; // both ADC and DAC
const byte POWER_OVERLOAD = 4;
const byte OVERHEAT = 8;
// current status
byte loadStatus = READY;
// </Status>

// <Operating Mode>
const byte MODE_CC = 0;
const byte MODE_CV = 1;
const byte MODE_CP = 2;
const byte MODE_CR = 3;
//  current mode
byte mode = MODE_CC;
// </Operating Mode>

// <Computed Values>
unsigned long power = 0;
unsigned long resistance = DVM_INPUT_RESISTANCE; // approx. input resistance of voltmeter
// </Computed Values>

void sendMessage(byte command);
void serialMonitor();

void setup()
{      
  delay(1000); // for safety
  Serial.begin(115200);
  #ifdef AVR
    analogReference(EXTERNAL);
  #endif
  #ifdef ARM
    ADC->ADC_MR |= (0b1111 << 24); // set maximum ADC tracking time
    analogReadResolution(12);
  #endif
  pinMode(REMOTE_PIN, OUTPUT);
  pinMode(V_GAIN_PIN, OUTPUT);
  pinMode(CV_PIN, OUTPUT);
  digitalWrite(REMOTE_PIN, LOW);
  setVrange(0);
  digitalWrite(CV_PIN, LOW);
  voltageRangeDown = getVRangeSwitching();

  I2C.begin();
  setI(0);

  // send identity message
  sendMessage(IDN);
}

void loop()
{

  // <Watchdog>
  watchdogCounter++;
  if ((watchdogCounter > WATCHDOG_TIMEOUT) && (isWatchdogEnabled == 1))
  {
    isWatchdogEnabled = 0;
    setI(0);
    setVrange(0);
    setRemote(0);
    Serial.end();
    Serial.begin(115200);
    loadStatus = READY;
  }
  // </Watchdog>

  getValues();
  controlLoop();
  serialMonitor();
}

void enableWatchdog()
{
  isWatchdogEnabled = 1;
  watchdogCounter = 0;  
}

void getValues()
{
  // temperature
  getTemperature();
  // voltage
  getVoltage();
  // current
  getCurrent();
  // power
  power = voltage;
  power *= current;
  power /= 1000;
  if (power > MAX_POWER)
  {
    setStatus(POWER_OVERLOAD);
  }
  // resistance
  if (current)
  {
    resistance = voltage;
    resistance *= 1000;
    resistance /= current;  
  }
  else
  {
    resistance = DVM_INPUT_RESISTANCE;
  }
}

void controlLoop()
{
  switch (mode)
  {
  case MODE_CC:
    {      
      // do nothing
      break;
    }
  case MODE_CV:
    {      
      // do nothing
      break;
    }
  case MODE_CP:
    {
      if (power < setPower)
      {
        plusCurrent();
      }
      else if (power > setPower)
      {
        minusCurrent();
      }
      break;
    } 
  case MODE_CR:
    {
      if (resistance < setResistance)
      {
        minusCurrent();
      }
      else if (resistance > setResistance)
      {
        plusCurrent();
      }
      break;
    }     
  }    
}


#include "terminal.h"

void getVoltage() // gets voltage and saves it to the global variable "voltage"
{
  unsigned int adcResult = readADC12bit(ADC_V_PIN);
  if (adcResult > 4080)
  {
    if (vRange == 0) // more than maximum range
    {
      setStatus(VOLTAGE_OVERLOAD);
    }
    else 
    {
      setVrange(0);
      getVoltage();
      return;
    }
  }

  long newVoltage = adcResult;
  if (vRange == 0)
  {
    // computation for larger range (no gain)
    newVoltage = ((newVoltage * VADC0_SLOPE) >> 12) + VADC0_INTERCEPT;
    if ((newVoltage < 0) || (adcResult == 0))
    {
      voltage = 0;
    }      
    else
    {
      voltage = newVoltage;
    }
    if ((voltage < voltageRangeDown) && (adcResult > 5))
    {
      setVrange(1); // next measurement will be in smaller (gain) range
    }
  }
  else
  {
    // computation for smaller range (with gain)
    newVoltage = ((newVoltage * VADC1_SLOPE) >> 12) + VADC1_INTERCEPT;
    if ((newVoltage < 0) || (adcResult < 10))
    {
      voltage = 0;
      #ifdef ARM
        setVrange(0); // against latching
      #endif
    }      
    else
    {
      voltage = newVoltage;
    }
  } 
}

void getCurrent() // gets current from ADC and saves it to the global variable "current"
{
  unsigned int adcResult = readADC12bit(ADC_I_PIN);
  if (adcResult > 4080)
  {
    setStatus(CURRENT_OVERLOAD);
  }
  long newCurrent = adcResult;
  newCurrent = ((newCurrent * IADC_SLOPE) >> 12) + IADC_INTERCEPT;
  if ((newCurrent < 0) || (adcResult == 0))
  {
    current = 0;
  }
  else
  {
    current = newCurrent;
  }
}

void getTemperature() // gets temperature from ADC and saves it to the global variable "temperature"
{
  int adc = analogRead(ADC_T_PIN);
  #ifdef AVR
    float t = 1/((log(THERMISTOR_R1*adc/(1024-adc))-log(THERMISTOR_R0))/THERMISTOR_B + 1/THERMISTOR_T0)-273.15;
  #endif
  #ifdef ARM
    float t = 1/((log(THERMISTOR_R1*adc/(4096-adc))-log(THERMISTOR_R0))/THERMISTOR_B + 1/THERMISTOR_T0)-273.15;
  #endif
  if (t < 0)
  {
    temperature = 0;
  }
  else if (t > 255)
  {
    temperature = 255;
  }
  else
  {
    temperature = t;
  }

  if (temperature > TEMPERATURE_THRESHOLD)
  {
    setStatus(OVERHEAT);
  }
}

unsigned int getVRangeSwitching()
{
  unsigned long Vadc = VADC1_SLOPE;
  Vadc = (((4080 * Vadc) >> 12) + VADC1_INTERCEPT) << 5;
  Vadc /= 33; // 97% of full-scale value
  return Vadc;
}

void setI(unsigned int value) // set current, unit = 1 mA
{  
  setMode(MODE_CC);
  setCurrent = value;
  long dacValue = value;
  dac = 0;

  if (setCurrent > 0)
  {
    dacValue = ((dacValue - IDAC_INTERCEPT) << 12) / IDAC_SLOPE;
  }

  if (dacValue > 4095)
  {
    setStatus(CURRENT_OVERLOAD);
  }
  else if (dacValue > 0)
  {
    dac = dacValue;
  }
  setDAC();
}

void plusCurrent()
{
  if (dac < 4095)
  {
    dac++;
  }
  setDAC();
}

void minusCurrent()
{
  if (dac > 0)
  {
    dac--;
  }
  setDAC();
}

void setV(unsigned int value)
{
  setMode(MODE_CV);
  setVoltage = value;
  long dacValue = value;
  if (vRange == 0)
  {
    // no gain
    dacValue = ((dacValue - VDAC0_INTERCEPT) << 12) / VDAC0_SLOPE;
  }
  else
  {
    // gain
    dacValue = ((dacValue - VDAC1_INTERCEPT) << 12) / VDAC1_SLOPE;
  }  
  
  if (dacValue > 4095)
  {
    if (vRange == 0)
    {
      dac = 4095;
      setDAC();
      setStatus(VOLTAGE_OVERLOAD);
    }
    else
    {
      setVrange(0);
      setV(setVoltage);
    }
  }
  else if (dacValue >= 0)
  {
    dac = dacValue;
    setDAC();
  }
}

void setMode(byte newMode)
{
  switch (newMode)
  {
  case MODE_CC:
    {
      if (mode == MODE_CV)
      {
        dac = 0;
        setDAC();
      }
      mode = MODE_CC;
      digitalWrite(CV_PIN, LOW);
      break;
    }
  case MODE_CV:
    {
      if (mode != MODE_CV)
      {
        dac = 4095;
        setDAC();
      }
      mode = MODE_CV;
      digitalWrite(CV_PIN, HIGH);
      break;
    }
  case MODE_CP:
    {
      if (mode == MODE_CV)
      {
        dac = 0;
        setDAC();
      }
      mode = MODE_CP;
      digitalWrite(CV_PIN, LOW);
      break;
    }
  case MODE_CR:
    {
      if (mode == MODE_CV)
      {
        dac = 0;
        setDAC();
      }
      mode = MODE_CR;   
      digitalWrite(CV_PIN, LOW);
      break;
    }      
  }
}

void setLoad(byte id, unsigned int val) // procedure called when there is a set (write to Arduino) data command
{  
  switch (id)
  {
  case MODE_CC:
    {
      setI(val);
      break;
    }
  case MODE_CV:
    {
      setV(val);
      break;
    }
  case MODE_CP:
    {
      setMode(MODE_CP);
      setPower = (long) val;
      break;
    }
  case MODE_CR:
    {
      setMode(MODE_CR);     
      setResistance = (long) val; 
      break;
    }         
  case REMOTE_ID:
   {
     setRemote(val >> 8);
     break;
   }
  }
}

void setRemote(byte value)
{
  if (value == 0) // local
  {
      digitalWrite(REMOTE_PIN, LOW);
      remoteStatus = 0;
  }
  else // remote
  {
      digitalWrite(REMOTE_PIN, HIGH);
      remoteStatus = 1;
  }
}

void setVrange(byte newRange)
{  
  // Disable Variable raning
  return;
   
  vRange = newRange; 
  if (newRange == 0)
  {
    digitalWrite(V_GAIN_PIN, LOW); // range 0
    if (mode == MODE_CV)
    {
      setV(setVoltage);
    }
  }
  else
  {    
    if (mode == MODE_CV)
    {
      setV(setVoltage);
    }
    digitalWrite(V_GAIN_PIN, HIGH); // range 1
  }
}

void setStatus(byte code)
{
  loadStatus |= code;
  if (code)
  {
    setI(0);
  }
}

void setDAC() // sets value to DAC
{ 
  I2C.beginTransmission(DAC_ADDRESS); 
  I2C.write(DAC_WRITE_DAC_AND_INPUT_REGISTERS);
  I2C.write((dac >> 4) & 0xFF);
  I2C.write((dac & 0xF) << 4);
  I2C.endTransmission();
  delayMicroseconds(8); // settling time  
}

unsigned int readADC12bit(int channel) // oversamples ADC to 12 bit (AVR) or averages 16 samples (ARM)
{  
  byte i;
  unsigned int analogResult = 0;
  #ifdef ARM
    delay(1);
  #endif
  for (i = 0; i < 16; i++)
  {
    analogResult += analogRead(channel);
  }
  #ifdef AVR
    return (analogResult >> 2) >> 1; // TODO: See why another shift is required
  #endif
  #ifdef ARM
    return (analogResult >> 4);
  #endif
}


