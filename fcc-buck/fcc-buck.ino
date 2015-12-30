//------------------------------------------------------------------------------------------------------
//
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
//
//------------------------------------------------------------------------------------------------------

#include <TimerOne.h>          // using Timer1 library from http://www.arduino.cc/playground/Code/Timer1
#include "lpf.h"               // Moving average low pass filter implementation

//------------------------------------------------------------------------------------------------------
// definitions
#define TRUE                1
#define FALSE               0
#define ON                  TRUE
#define OFF                 FALSE

#define PWM_PIN             9             // the output pin for the pwm, connected to OC1A
#define PWM_ENABLE_PIN      8             // pin used to control shutoff function of the IR2104 MOSFET driver
#define TURN_ON_MOSFETS     digitalWrite(PWM_ENABLE_PIN, HIGH)     // enable MOSFET driver
#define TURN_OFF_MOSFETS    digitalWrite(PWM_ENABLE_PIN, LOW)      // disable MOSFET driver

#define ADC_SOL_AMPS_CHAN   1             // the adc channel to read solar amps
#define ADC_SOL_VOLTS_CHAN  0             // the adc channel to read solar volts
#define ADC_BAT_VOLTS_CHAN  2             // the adc channel to read battery volts

#define PWM_FULL            1023          // the actual value used by the Timer1 routines for 100% pwm duty cycle
#define PWM_MAX             0.9*PWM_FULL  // the max value for pwm duty cyle 0-100.0% (Resolution in 
#define PWM_MIN             0.1*PWM_FULL  // the min value for pwm duty cyle 0-100.0%
#define PWM_START           0.1*PWM_FULL  // the starting value for pwm duty cyle 0-100.0%

// MPPT configuration is in mppt.h

#define AVG_NUM             8             // number of iterations of the adc routine to average the adc readings
#define SOL_AMPS_SCALE      0.5           // the scaling value for raw adc reading to get solar amps scaled by 100 [(1/(0.005*(3.3k/25))*(5/1023)*100]
#define SOL_VOLTS_SCALE     2.7           // the scaling value for raw adc reading to get solar volts scaled by 100 [((10+2.2)/2.2)*(5/1023)*100]
#define BAT_VOLTS_SCALE     2.7           // the scaling value for raw adc reading to get battery volts scaled by 100 [((10+2.2)/2.2)*(5/1023)*100]

#define ONE_SECOND          50000         // count for number of interrupt in 1 second on interrupt period of 20us

#define PIN_LED             13            // LED connected to digital pin 13
#define BUFF_MAX            16            // Maximum buffer size for receiving from serial

//------------------------------------------------------------------------------------------------------
// global variables

// enumerated variable that holds state for charger state machine
enum charger_mode_t {MODE_OFF,            // The system is off
                     MODE_MPPT_ON,        // MPPT mode
                     MODE_MPPT_OFF,       // MPPT auto-off mode
                     MODE_CONST_VOLT,     // Constant voltage mode
                     MODE_CONST_CURRENT,  // Constant current mode
                     MODE_CONST_POWER,    // Constant power mode
                     MODE_CONST_DUTY      // Constant duty cycle mode
                    };

struct system_states_t {
   int sol_amps;                         // solar amps in 10 mV (scaled by 100)
   int sol_volts;                        // solar volts in 10 mV (scaled by 100)
   int bat_volts;                        // battery volts in 10 mV (scaled by 100)
   int sol_watts;                        // solar watts in 10 mV (scaled by 100)
   uint16_t pwm_duty;                    // pwm target duty cycle, set by set_pwm_duty function
   int target;                           // voltage or power target
   charger_mode_t mode;                  // state of the charge state machine
} power_status;

unsigned int g_seconds = 0;             // seconds from timer routine

//------------------------------------------------------------------------------------------------------
typedef LowPassBuffer<16, unsigned int> LPF;
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

  lpf_in_volts.AddData ( analogRead(ADC_SOL_VOLTS_CHAN) );
  lpf_out_volts.AddData( analogRead(ADC_BAT_VOLTS_CHAN) );
  lpf_in_amps.AddData  ( analogRead(ADC_SOL_AMPS_CHAN) );

  power_status.sol_amps =  SOL_AMPS_SCALE * lpf_in_amps.GetAverage();
  power_status.sol_volts = SOL_VOLTS_SCALE * lpf_in_volts.GetAverage();
  power_status.bat_volts = BAT_VOLTS_SCALE * lpf_out_volts.GetAverage();
  power_status.sol_watts = (int)((((long)power_status.sol_amps * (long)power_status.sol_volts) + 50) / 100);    //calculations of solar watts scaled by 10000 divide by 100 to get scaled by 100                 

//  power_status.sol_amps =  SOL_AMPS_SCALE * read_adc(ADC_SOL_AMPS_CHAN) ;// * read_adc(ADC_SOL_AMPS_CHAN); //((read_adc(ADC_SOL_AMPS_CHAN)  * SOL_AMPS_SCALE) + 5) / 10;    //input of solar amps result scaled by 100
//  power_status.sol_volts = SOL_VOLTS_SCALE * read_adc(ADC_SOL_VOLTS_CHAN); //((read_adc(ADC_SOL_VOLTS_CHAN) * SOL_VOLTS_SCALE) + 5) / 10;   //input of solar volts result scaled by 100
//  power_status.bat_volts = BAT_VOLTS_SCALE * read_adc(ADC_BAT_VOLTS_CHAN); // ((read_adc(ADC_BAT_VOLTS_CHAN) * BAT_VOLTS_SCALE) + 5) / 10;   //input of battery volts result scaled by 100
//  power_status.sol_watts = (int)((((long)power_status.sol_amps * (long)power_status.sol_volts) + 50) / 100);    //calculations of solar watts scaled by 10000 divide by 100 to get scaled by 100                 
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
    Timer1.pwm(PWM_PIN,pwm, 20);                           // use Timer1 routine to set pwm duty cycle at 20uS period
    //Timer1.pwm(PWM_PIN,(PWM_FULL * (long)pwm / 100));
  }												
  else if (pwm == PWM_MAX) {				                      // if pwm set to 100% it will be on full but we have 
    Timer1.pwm(PWM_PIN,(PWM_FULL - 1), 1000);             // keep switching so set duty cycle at 99.9% and slow down to 1000uS period 
    //Timer1.pwm(PWM_PIN,(PWM_FULL - 1));              
  }												
}		

//------------------------------------------------------------------------------------------------------
// MPPT Algorithm
//------------------------------------------------------------------------------------------------------
#include "mppt.h"

//------------------------------------------------------------------------------------------------------
// PWM Adjustments for constant voltage and constant power modes.
// If "target_difference" > 0, it increases PWM
// If "target_difference" < 0, it decreases PWM
// The PWM step adjustment depends on the difference between target and current voltage/power
//------------------------------------------------------------------------------------------------------
void adjust_pwm(int target_difference) {

  int current_pwm = power_status.pwm_duty;

  // proportional delta control
  int pwm_delta = 1 + (abs(target_difference) / 25);
  
  // adjust pwm either up or down based on difference sign
  if (target_difference < 0) {
    current_pwm -= pwm_delta;
  } else if (target_difference > 0) {
    current_pwm += pwm_delta;
  }
  
  set_pwm_duty(current_pwm);
}

//------------------------------------------------------------------------------------------------------
// Switch state to the mode, to reach the given target
// mode: A charger_mode_t constant
// target: Value to reach in 0.01 of the unit (10 mVs, 10 mAs, 10 mWs, etc.)
//------------------------------------------------------------------------------------------------------
void state_switch(charger_mode_t mode, int target = 0) {

  switch (mode) {
    case MODE_OFF:
      TURN_OFF_MOSFETS;
      power_status.mode = mode;
      break;
      
    case MODE_CONST_VOLT:
    case MODE_CONST_CURRENT:
    case MODE_CONST_POWER:
      TURN_ON_MOSFETS;
      power_status.mode = mode;
      power_status.target = target;
      break;

    case MODE_CONST_DUTY:
      power_status.mode = mode;
      power_status.target = target;
      set_pwm_duty((long)target * (PWM_FULL - 1) / 10000);
      TURN_ON_MOSFETS;
      break;

    case MODE_MPPT_ON:
      TURN_ON_MOSFETS;
      power_status.mode = mode;
      break;

    case MODE_MPPT_OFF:
      TURN_OFF_MOSFETS;
      power_status.mode = mode;
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------------------------------
// The program State Machine, called from the main loop
//------------------------------------------------------------------------------------------------------
void state_machine(void) {
  switch (power_status.mode) {
    case MODE_CONST_VOLT:
      adjust_pwm(power_status.target - power_status.bat_volts);
      break;
    case MODE_CONST_CURRENT:
      adjust_pwm(power_status.target - power_status.sol_amps);
      break;
    case MODE_CONST_POWER:
      adjust_pwm(power_status.target - power_status.sol_watts);
      break;
    case MODE_CONST_DUTY:
      break; // nothing to be done in constant duty pwm mode, as duty cycle is already set in "state_switch"
    case MODE_OFF:
      break; // do nothing if it is in off mode
    case MODE_MPPT_ON: 
    case MODE_MPPT_OFF:
      MPPT_state_machine(); // call mppt state-machine
    default: // other cases: do nothing
      break;
  }
}

//------------------------------------------------------------------------------------------------------
// Termianl communication functions
//------------------------------------------------------------------------------------------------------
#include "terminal.h"

//------------------------------------------------------------------------------------------------------
// This routine is automatically called at powerup/reset
//------------------------------------------------------------------------------------------------------
void setup()                               // run once, when the sketch starts
{
  Serial.begin(115200);                    // open the serial port at 115200 bps:

  Timer1.initialize(20);                   // initialize timer1, and set a 20uS period
  Timer1.pwm(PWM_PIN, 0);                  // setup pwm on pin 9, 0% duty cycle
  Timer1.attachInterrupt(timer_callback);  // attaches timer_callback() as a timer overflow interrupt

  pinMode(PWM_ENABLE_PIN, OUTPUT);         // sets the digital pin as output
  TURN_OFF_MOSFETS;                        // turn on MOSFET driver chip
  set_pwm_duty(PWM_START);

  lpf_in_volts.Init();                     // initialize ADC low-pass filters
  lpf_out_volts.Init();
  lpf_in_amps.Init();

  state_switch(MODE_OFF);                     // start with charger state as off

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
  if (Serial.available() >= 4) {
    serve_serial_command();              // read commands from serial
  }
}
//------------------------------------------------------------------------------------------------------
