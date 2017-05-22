//------------------------------------------------------------------------------------------------------
// Boost/Battery charger controller
// Part of Free charge controller project
//
// Based on:
// Arduino Peak Power Tracking Solar Charger     by Tim Nolan (www.timnolan.com)   5/1/09
//
//    This software implements my Peak Power Tracking Solar Charger using the Arduino Demilove developement
//    board. I'm releasing this software and hardware project as open source. It is free of any restiction for
//    anyone to use. All I ask is that if you use any of my hardware or software or ideas from this project
//    is that you give me credit and add a link to my website www.timnolan.com to your documentation. 
//    Thank you.
//
//    5/1/09  v1.00    First development version. Just getting something to work.
//
//------------------------------------------------------------------------------------------------------

#include <TimerOne.h>          // using Timer1 library from http://www.arduino.cc/playground/Code/Timer1
#include "lpf.h"               // Moving average low pass filter implementation

//------------------------------------------------------------------------------------------------------
// definitions
#define PWM_PIN             9             // the output pin for the pwm, connected to OC1A
#define PWM_ENABLE_PIN      8             // pin used to control shutoff function of the IR2104 MOSFET driver
#define TURN_ON_MOSFETS     digitalWrite(PWM_ENABLE_PIN, HIGH)     // enable MOSFET driver
#define TURN_OFF_MOSFETS    digitalWrite(PWM_ENABLE_PIN, LOW)      // disable MOSFET driver

#define ADC_IN_AMPS_CHAN    1             // the adc channel to read input (solar) amps
#define ADC_IN_VOLTS_CHAN   0             // the adc channel to read input (solar) volts
#define ADC_OUT_VOLTS_CHAN  2             // the adc channel to read output (battery) volts

#define PWM_FULL            1023          // the actual value used by the Timer1 routines for 100% pwm duty cycle
#define PWM_MAX             0.6*PWM_FULL  // the max value for pwm duty cyle 0-100.0%
#define PWM_MIN             0.01*PWM_FULL // the min value for pwm duty cyle 0-100.0%
#define PWM_START           PWM_MIN       // the starting value for pwm duty cyle 0-100.0%

// Battery configuration is in charger.h

// Use these to calibrate the ADC
#define IN_AMPS_SCALE       0.5           // the scaling value for raw adc reading to get input (battery) amps scaled by 100 [(1/(0.005*(3.3k/25))*(5/1023)*100]
#define IN_VOLTS_SCALE      2.7           // the scaling value for raw adc reading to get input (battery) volts scaled by 100 [((10+2.2)/2.2)*(5/1023)*100]
#define OUT_VOLTS_SCALE     2.7           // the scaling value for raw adc reading to get output (load) volts scaled by 100 [((10+2.2)/2.2)*(5/1023)*100]

// Safety Limits
#define SAFETY_MAX_IN_CURRENT  300        // Maximum input current to enable safety limit (3 A)
#define SAFETY_MAX_IN_POWER    1500       // Maximum input current to enable safety limit (15 watts)

#define SERIAL_BUFF_MAX        32         // Maximum buffer size for receiving from serial

#define ONE_SECOND             50000      // count for number of interrupt in 1 second on interrupt period of 20us
#define STATE_MACHINE_SKIPS    32         // State machine skip counter, gives some time to the voltage LPF to adapt to 

typedef LowPassBuffer<16, unsigned int> LPF; // Low pass filter uses a moving average window of 16 samples
#define PWM_PID_P	           25         // PWM PID filter proportional divisor

//------------------------------------------------------------------------------------------------------
// global variables

// enumerated variable that holds state for charger state machine
enum charger_mode_t {MODE_OFF,            // The system is off
                     MODE_CONST_VOLT,     // Constant voltage mode
                     MODE_CONST_CURRENT,  // Constant current mode
                     MODE_CONST_POWER,    // Constant power mode
                     MODE_CONST_DUTY,     // Constant duty cycle mode
                     MODE_BATT_OFF,       // Battery off mode
                     MODE_BATT_BULK,      // Battery bulk (constant current) charging mode
                     MODE_BATT_FLOAT,     // Battery float (constant voltage) charging mode
                    };

enum safety_error_t { LIMIT_DISABLED,     // Safety system in disabled
                      LIMIT_NORMAL,       // Safety system in normal mode
                      LIMIT_VOLTAGE,      // Over-voltage detected
                      LIMIT_CURRENT,      // Over-current detected
                      LIMIT_POWER         // Over-power detected
                    };

struct system_states_t {
   int in_amps;                          // input (solar) amps in 10 mV (scaled by 100)
   int in_volts;                         // input (solar) volts in 10 mV (scaled by 100)
   int out_volts;                        // output (battery) volts in 10 mV (scaled by 100)
   int in_watts;                         // input (solar) watts in 10 mV (scaled by 100)

   uint16_t pwm_duty;                    // pwm target duty cycle, set by set_pwm_duty function
   int target;                           // voltage, current or power target
   charger_mode_t mode;                  // state of the charge state machine

   safety_error_t safety_limit_status;   // Safety limit status
   bool safety_limit_enabled;            // Safety limit enabled flag
} power_status;

unsigned int g_seconds = 0;             // seconds from timer routine

LPF lpf_in_volts;
LPF lpf_in_amps;
LPF lpf_out_volts;

//------------------------------------------------------------------------------------------------------
// This is interrupt service routine for Timer1 that occurs every 20uS.
// It is only used to incremtent the seconds counter. 
// Timer1 is also used to generate the pwm output.
//------------------------------------------------------------------------------------------------------
void timer_callback()
{
  static unsigned int interrupt_counter = 0;     // counter for 20us interrrupt

  if (interrupt_counter++ > ONE_SECOND) {        //increment interrupt_counter until one second has passed
    interrupt_counter = 0;  
    g_seconds++;                                   //then increment seconds counter
  }
}

//------------------------------------------------------------------------------------------------------
// This routine reads all the analog input values for the system. Then it multiplies them by the scale
// factor to get actual value in volts or amps. Then it adds on a rounding value before dividing to get
// the result scaled by 100 to give a fractional value of two decimal places. It also calculates the input
// watts from the solar amps times the solar voltage and rounds and scales that by 100 (2 decimal places) also.
//------------------------------------------------------------------------------------------------------
void read_data(void) {

  lpf_in_volts.AddData ( analogRead(ADC_IN_VOLTS_CHAN) );
  lpf_out_volts.AddData( analogRead(ADC_OUT_VOLTS_CHAN) );
  lpf_in_amps.AddData  ( analogRead(ADC_IN_AMPS_CHAN) );

  power_status.in_amps =  IN_AMPS_SCALE * lpf_in_amps.GetAverage();
  power_status.in_volts = IN_VOLTS_SCALE * lpf_in_volts.GetAverage();
  power_status.out_volts = OUT_VOLTS_SCALE * lpf_out_volts.GetAverage();
  power_status.in_watts = (int)((((long)power_status.in_amps * (long)power_status.in_volts) + 50) / 100);    //calculations of solar watts scaled by 10000 divide by 100 to get scaled by 100                 
}
//------------------------------------------------------------------------------------------------------
// This routine uses the Timer1.pwm function to set the pwm duty cycle. The routine takes the value in
// the variable pwm as 0-100 duty cycle and scales it to get 0-1034 for the Timer1 routine. 
// There is a special case for 100% duty cycle. Normally this would be have the top MOSFET on all the time
// but the MOSFET driver IR2104 uses a charge pump to generate the gate voltage so it has to keep running 
// all the time. So for 100% duty cycle I set the pwm value to 1023 - 1 so it is on 99.9% almost full on 
// but is switches enough to keep the charge pump on IR2104 working.
//------------------------------------------------------------------------------------------------------

void set_pwm_duty(int pwm) {

  if (pwm > PWM_MAX) {					   // check limits of PWM duty cyle and set to PWM_MAX
    pwm = PWM_MAX;		
  }
  else if (pwm < PWM_MIN) {				   // if pwm is less than PWM_MIN then set it to PWM_MIN
    pwm = PWM_MIN;
  }
  
  power_status.pwm_duty = pwm;                             // store this value in the status
  
  if (pwm < PWM_MAX) {
    Timer1.pwm(PWM_PIN, PWM_FULL - pwm, 20);  // use Timer1 routine to set pwm duty cycle at 20uS period
    //Timer1.pwm(PWM_PIN,(PWM_FULL * (long)pwm / 100));
  }												
  else if (pwm == PWM_MAX) {                  // if pwm set to 100% it will be on full but we have 
    Timer1.pwm(PWM_PIN, 4, 1000);             // keep switching so set duty cycle at 99.9% and slow down to 1000uS period 
    //Timer1.pwm(PWM_PIN,(PWM_FULL - 1));              
  }												
}

//------------------------------------------------------------------------------------------------------
// PWM Adjustments for constant voltage and constant power modes.
// If "target_difference" > 0, it increases PWM
// If "target_difference" < 0, it decreases PWM
// The PWM step adjustment depends on the difference between target and current voltage/power
//------------------------------------------------------------------------------------------------------
void adjust_pwm(int target_difference) {

  int current_pwm = power_status.pwm_duty;

  // proportional delta control
  int pwm_delta = 1 + (abs(target_difference) / PWM_PID_P);
  
  // adjust pwm either up or down based on difference sign
  if (target_difference < 0) {
    current_pwm -= pwm_delta;
  } else if (target_difference > 0) {
    current_pwm += pwm_delta;
  }
  
  set_pwm_duty(current_pwm);
}

//------------------------------------------------------------------------------------------------------
// Safety limit enable/disable
// enable: true to enable safety limit mode, false to disable
//------------------------------------------------------------------------------------------------------
void safety_limit_enable(bool enable) {
  power_status.safety_limit_enabled = enable;      

  if (enable) {
    power_status.safety_limit_status = LIMIT_NORMAL;     // Safety limit no error
  } else {
    power_status.safety_limit_status = LIMIT_DISABLED;
  }
}

//------------------------------------------------------------------------------------------------------
// Safety limit handler
// Adjusts PWM to remain under the current and power limits
// returns: safety_error_t if anything has happened
//------------------------------------------------------------------------------------------------------
safety_error_t  safety_limit_pwm() {

    if (power_status.in_amps > SAFETY_MAX_IN_CURRENT) {
        adjust_pwm(SAFETY_MAX_IN_CURRENT - power_status.in_amps);
        return LIMIT_CURRENT;
    }

    if (power_status.in_watts > SAFETY_MAX_IN_POWER) {
        adjust_pwm(SAFETY_MAX_IN_POWER - power_status.in_watts);
        return LIMIT_POWER;
    }

    // no limits
    return LIMIT_NORMAL;
}

//------------------------------------------------------------------------------------------------------
// Battery Charger Function
//------------------------------------------------------------------------------------------------------
#include "charger.h"

//------------------------------------------------------------------------------------------------------
// Switch state to the mode, to reach the given target
// mode: A charger_mode_t constant
// target: Value to reach in 0.01 of the unit (10 mVs, 10 mAs, 10 mWs, etc.)
//------------------------------------------------------------------------------------------------------
void state_switch(const charger_mode_t & mode, int target = 0) {

  switch (mode) {
    case MODE_OFF:
      TURN_OFF_MOSFETS;
      power_status.mode = mode;
      set_pwm_duty(PWM_START); // set pwm value to something safe
      break;
      
    case MODE_CONST_VOLT:
    case MODE_CONST_CURRENT:
    case MODE_CONST_POWER:
      power_status.mode = mode;
      power_status.target = target;
      TURN_ON_MOSFETS;
      break;

    case MODE_CONST_DUTY:
      power_status.mode = mode;
      power_status.target = target;
      set_pwm_duty((long)target * (PWM_FULL - 1) / 10000);
      TURN_ON_MOSFETS;
      break;

    case MODE_BATT_BULK:
    case MODE_BATT_FLOAT:
    case MODE_BATT_OFF:
      Charger_switch_mode(mode);
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------------------------------
// The program State Machine, called from the main loop
//------------------------------------------------------------------------------------------------------
void state_machine(void) {

  // If safety mode is enabled, check and adjust PWM
  if (power_status.safety_limit_enabled) {
    power_status.safety_limit_status = safety_limit_pwm();

    if (power_status.safety_limit_status != LIMIT_NORMAL) {  // Do nothing else if over the limit
      return;
    }
  }

  // Skip adjusting pwm, to let time for the ADC LPF to catchup
  static unsigned int skip_counter = 0;
  skip_counter++;
  if (skip_counter < STATE_MACHINE_SKIPS) return;
  skip_counter = 0;

  // adjust pwm based on current mode
  switch (power_status.mode) {
    case MODE_CONST_VOLT:
      adjust_pwm(power_status.target - power_status.out_volts);
      break;
    case MODE_CONST_CURRENT:
      adjust_pwm(power_status.target - power_status.in_amps);
      break;
    case MODE_CONST_POWER:
      adjust_pwm(power_status.target - power_status.in_watts);
      break;
    case MODE_CONST_DUTY:
      break; // nothing to be done in constant duty pwm mode, as duty cycle is already set in "state_switch"
    case MODE_BATT_OFF:
    case MODE_BATT_BULK:
    case MODE_BATT_FLOAT:
      Charger_State_Machine();
      break;
    case MODE_OFF: // do nothing if it is in off mode
    default: // other cases: do nothing
      break;
  }
}

//------------------------------------------------------------------------------------------------------
// Termianl communication functions
//------------------------------------------------------------------------------------------------------
#include "terminal.h"

//------------------------------------------------------------------------------------------------------
// Setup function
// This routine is automatically called at powerup/reset
//------------------------------------------------------------------------------------------------------
void setup()                               // run once, when the sketch starts
{
  Serial.begin(115200);                    // open the serial port at 115200 bps

  Timer1.initialize(20);                   // initialize timer1, and set a 20uS period
  Timer1.pwm(PWM_PIN, 0);                  // setup pwm on pin 9, 0% duty cycle
  Timer1.attachInterrupt(timer_callback);  // attaches timer_callback() as a timer overflow interrupt

  pinMode(PWM_ENABLE_PIN, OUTPUT);         // sets the digital pin as output

  state_switch(MODE_OFF);                  // start with charger state as off

  safety_limit_enable(true);               // Safety limit error

  lpf_in_volts.Init();                     // initialize ADC low-pass filters
  lpf_out_volts.Init();
  lpf_in_amps.Init();

  print_identity();
}

//------------------------------------------------------------------------------------------------------
// Main loop.
// Right now the number of times per second that this main loop runs is set by how long the printing to 
// the serial port takes. You can speed that up by speeding up the baud rate.
// You can also run the commented out code and the charger routines will run once a second.
//------------------------------------------------------------------------------------------------------

void loop()                          // run over and over again
{
  read_data();                         //read data from sensors
  
  state_machine();                     //run the state machine

  // serve commands from serial port
  if (Serial.available() >= 7) {         // {"X":0} is 7 bytes long
    serve_serial_command();              // read commands from serial
  }
}
//------------------------------------------------------------------------------------------------------
