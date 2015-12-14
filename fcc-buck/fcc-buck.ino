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
#define PWM_MAX             0.9*PWM_FULL  // the value for pwm duty cyle 0-100.0% (Resolution in 
#define PWM_MIN             0.1*PWM_FULL  // the value for pwm duty cyle 0-100.0%
#define PWM_START           0.1*PWM_FULL  // the value for pwm duty cyle 0-100.0%

#define PWM_INC             4             // the value the increment to the pwm value for the ppt algorithm

#define AVG_NUM             8             // number of iterations of the adc routine to average the adc readings
#define SOL_AMPS_SCALE      0.5           // the scaling value for raw adc reading to get solar amps scaled by 100 [(1/(0.005*(3.3k/25))*(5/1023)*100]
#define SOL_VOLTS_SCALE     2.7           // the scaling value for raw adc reading to get solar volts scaled by 100 [((10+2.2)/2.2)*(5/1023)*100]
#define BAT_VOLTS_SCALE     2.7           // the scaling value for raw adc reading to get battery volts scaled by 100 [((10+2.2)/2.2)*(5/1023)*100]

#define ONE_SECOND          50000         // count for number of interrupt in 1 second on interrupt period of 20us
#define LOW_SOL_WATTS       500           // value of solar watts scaled by 100 so this is 5.00 watts
#define MIN_SOL_WATTS       100           // value of solar watts scaled by 100 so this is 1.00 watts
#define MIN_BAT_VOLTS       1100          // value of battery voltage scaled by 100 so this is 11.00 volts          
#define MAX_BAT_VOLTS       1410          // value of battery voltage scaled by 100 so this is 14.10 volts  
#define HIGH_BAT_VOLTS      1300          // value of battery voltage scaled by 100 so this is 13.00 volts  
#define SOL_BAT_VOLT_DIFF   200           // For modified MPPT to start, solar voltage must be this much higher than battery side
#define OFF_NUM             9             // number of iterations of off charger state
  
#define PIN_LED             13            // LED connected to digital pin 13
#define BUFF_MAX            16            // Maximum buffer size for receiving from serial

//------------------------------------------------------------------------------------------------------
// global variables

// enumerated variable that holds state for charger state machine
enum charger_mode_t {MODE_OFF,            // The system is off
                     MODE_MPPT_ON,        // MPPT mode
                     MODE_MPPT_OFF,       // MPPT auto-off mode
                     MODE_MPPT_BULK,      // MPPT bulck charger
                     MODE_MPPT_BAT_FLOAT, // MPPT batt float mode
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

unsigned int seconds = 0;             // seconds from timer routine
  
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
    seconds++;                                   //then increment seconds counter
  }
}

//-----------------------------------------------------------------------------------
// This function prints int that was scaled by 100 with 2 decimal places
//-----------------------------------------------------------------------------------
void print_int100_dec2(int temp) {

  Serial.print(temp/100,DEC);        // divide by 100 and print interger value
  Serial.print(".");
  if ((temp%100) < 10) {             // if fractional value has only one digit
    Serial.print("0");               // print a "0" to give it two digits
    Serial.print(temp%100,DEC);      // get remainder and print fractional value
  }
  else {
    Serial.print(temp%100,DEC);      // get remainder and print fractional value
  }
}
//------------------------------------------------------------------------------------------------------
// This routine prints all the data out to the serial port.
//------------------------------------------------------------------------------------------------------
void print_data(void) {
  
  Serial.print(seconds,DEC);
  Serial.print("  ");

  Serial.print("charger = ");
  if (power_status.mode == MODE_OFF)                 Serial.print("off    ");
  else if (power_status.mode == MODE_MPPT_ON)        Serial.print("mppton ");
  else if (power_status.mode == MODE_MPPT_OFF)       Serial.print("autooff");
  else if (power_status.mode == MODE_MPPT_BULK)      Serial.print("bulk   ");
  else if (power_status.mode == MODE_MPPT_BAT_FLOAT) Serial.print("float  ");
  else if (power_status.mode == MODE_CONST_VOLT)     Serial.print("volt   ");
  else if (power_status.mode == MODE_CONST_CURRENT)  Serial.print("amps   ");
  else if (power_status.mode == MODE_CONST_POWER)    Serial.print("watt   ");
  else if (power_status.mode == MODE_CONST_DUTY)     Serial.print("duty   ");
  Serial.print("  ");

  Serial.print("target = ");
  print_int100_dec2(power_status.target);

  Serial.print("pwm = ");
  print_int100_dec2((long)power_status.pwm_duty * 10000 / (PWM_FULL - 1));
  Serial.print("  ");

  Serial.print("s_amps = ");
  print_int100_dec2(power_status.sol_amps);
  Serial.print("  ");

  Serial.print("s_volts = ");
  print_int100_dec2(power_status.sol_volts);
  Serial.print("  ");

  Serial.print("s_watts = ");
  print_int100_dec2(power_status.sol_watts);
  Serial.print("  ");

  Serial.print("b_volts = ");
  print_int100_dec2(power_status.bat_volts);
  Serial.print("  ");

  Serial.print("\n\r");
}

void print_data_json(void) {
  Serial.print("{\"time\": ");
  Serial.print(seconds, DEC);

  Serial.print(", \"state\": ");
  if (power_status.mode == MODE_OFF)                 Serial.print("\"off\",    ");
  else if (power_status.mode == MODE_MPPT_ON)        Serial.print("\"mppton\", ");
  else if (power_status.mode == MODE_MPPT_OFF)       Serial.print("\"mpptoff\",");
  else if (power_status.mode == MODE_MPPT_BULK)      Serial.print("\"bulk\",   ");
  else if (power_status.mode == MODE_MPPT_BAT_FLOAT) Serial.print("\"float\",  ");
  else if (power_status.mode == MODE_CONST_VOLT)     Serial.print("\"volt\",   ");
  else if (power_status.mode == MODE_CONST_CURRENT)  Serial.print("\"amps\",   ");
  else if (power_status.mode == MODE_CONST_POWER)    Serial.print("\"watt\",   ");
  else if (power_status.mode == MODE_CONST_DUTY)     Serial.print("\"duty\",   ");

  Serial.print(" \"target\": ");
  print_int100_dec2(power_status.target);

  Serial.print(", \"pwm\": ");
  print_int100_dec2((long)power_status.pwm_duty * 10000 / (PWM_FULL - 1));

  Serial.print(", \"volts_in\": ");
  print_int100_dec2(power_status.sol_volts);

  Serial.print(", \"volts_out\": ");
  print_int100_dec2(power_status.bat_volts);

  Serial.print(", \"amps_in\": ");
  print_int100_dec2(power_status.sol_amps);

  Serial.print(", \"watts_in\": ");
  print_int100_dec2(power_status.sol_watts);

  Serial.println("}");
}
//------------------------------------------------------------------------------------------------------
// Prints device identity as a JSON string
//------------------------------------------------------------------------------------------------------
void print_identity() 
{
    Serial.println("{\"device\": \"FreeChargeControlBuck\", \"version\":1}");
}

//------------------------------------------------------------------------------------------------------
// This routine reads all the analog input values for the system. Then it multiplies them by the scale
// factor to get actual value in volts or amps. Then it adds on a rounding value before dividing to get
// the result scaled by 100 to give a fractional value of two decimal places. It also calculates the input
// watts from the solar amps times the solar voltage and rounds and scales that by 100 (2 decimal places) also.
//------------------------------------------------------------------------------------------------------
void read_data(void) {

  lpf_in_volts.AddData(  analogRead(ADC_SOL_VOLTS_CHAN) );
  lpf_out_volts.AddData( analogRead(ADC_BAT_VOLTS_CHAN) );
  lpf_in_amps.AddData(   analogRead(ADC_SOL_AMPS_CHAN) );

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
// This routine is the charger state machine. It has four states on, off, bulk and float.
// It's called once each time through the main loop to see what state the charger should be in.
// The battery charger can be in one of the following four states:
// 
//  On State - this is charger state for MIN_SOL_WATTS < solar watts < LOW_SOL_WATTS. This state is probably
//      happening at dawn and dusk when the solar watts input is too low for the bulk charging state but not
//      low enough to go into the off state. In this state we just set the pwm = 100% to get the most of low
//      amount of power available.
//  Bulk State - this is charger state for solar watts > MIN_SOL_WATTS. This is where we do the bulk of the battery
//      charging and where we run the Peak Power Tracking alogorithm. In this state we try and run the maximum amount
//      of current that the solar panels are generating into the battery.
//  Float State - As the battery charges it's voltage rises. When it gets to the MAX_BAT_VOLTS we are done with the 
//      bulk battery charging and enter the battery float state. In this state we try and keep the battery voltage
//      at MAX_BAT_VOLTS by adjusting the pwm value. If we get to pwm = 100% it means we can't keep the battery 
//      voltage at MAX_BAT_VOLTS which probably means the battery is being drawn down by some load so we need to back
//      into the bulk charging mode.
//  Off State - This is state that the charger enters when solar watts < MIN_SOL_WATTS. The charger goes into this
//      state when it gets dark and there is no more power being generated by the solar panels. The MOSFETs are turned
//      off in this state so that power from the battery doesn't leak back into the solar panel. When the charger off
//      state is first entered all it does is decrement off_count for OFF_NUM times. This is done because if the battery
//      is disconnected (or battery fuse is blown) it takes some time before the battery voltage changes enough so we can tell
//      that the battery is no longer connected. This off_count gives some time for battery voltage to change so we can
//      tell this.
//------------------------------------------------------------------------------------------------------

void MPPT_original_state_machine(void) {
  
  static int old_sol_watts = 0;
  static int off_count = OFF_NUM;
  static int delta = PWM_INC;                   // variable used to modify pwm duty cycle for the ppt algorithm

  int pwm = power_status.pwm_duty;
  int sol_watts = power_status.sol_watts;
  int bat_volts = power_status.bat_volts;
  int sol_volts = power_status.sol_volts;
  
  switch (power_status.mode) {
    case MODE_MPPT_ON:                                        
      if (sol_watts < MIN_SOL_WATTS) {          //if watts input from the solar panel is less than
        power_status.mode = MODE_MPPT_OFF;      //the minimum solar watts then it is getting dark so
        off_count = OFF_NUM;                    //go to the charger off state
        TURN_OFF_MOSFETS; 
      }
      else if (bat_volts > MAX_BAT_VOLTS) {       //else if the battery voltage has gotten above the float
        power_status.mode = MODE_MPPT_BAT_FLOAT;  //battery float voltage go to the charger battery float state
      }
      else if (sol_watts < LOW_SOL_WATTS) {     //else if the solar input watts is less than low solar watts
        pwm = PWM_MAX;                          //it means there is not much power being generated by the solar panel
        set_pwm_duty(pwm);			                //so we just set the pwm = 100% so we can get as much of this power as possible
      }                                         //and stay in the charger on state
      else {                                          
        pwm = ((bat_volts * 10) / (sol_volts / 10)) + 5;  //else if we are making more power than low solar watts figure out what the pwm
        power_status.mode = MODE_MPPT_BULK;               //value should be and change the charger to bulk state 
      }
      break;
    case MODE_MPPT_BULK:
      if (sol_watts < MIN_SOL_WATTS) {         //if watts input from the solar panel is less than
        power_status.mode = MODE_MPPT_OFF;     //the minimum solar watts then it is getting dark so
        off_count = OFF_NUM;                   //go to the charger off state
        TURN_OFF_MOSFETS; 
      }
      else if (bat_volts > MAX_BAT_VOLTS) {      //else if the battery voltage has gotten above the float
        power_status.mode = MODE_MPPT_BAT_FLOAT; //battery float voltage go to the charger battery float state
      }
      else if (sol_watts < LOW_SOL_WATTS) {    //else if the solar input watts is less than low solar watts
        power_status.mode = MODE_MPPT_ON;      //it means there is not much power being generated by the solar panel
        TURN_ON_MOSFETS;                       //so go to charger on state
      }
      else {                                   // this is where we do the Peak Power Tracking ro Maximum Power Point algorithm
        if (old_sol_watts >= sol_watts) {      // if previous watts are greater change the value of
          delta = -delta;			                 // delta to make pwm increase or decrease to maximize watts
        }
        pwm += delta;                          // add delta to change PWM duty cycle for PPT algorythm 
        old_sol_watts = sol_watts;             // load old_watts with current watts value for next time
        set_pwm_duty(pwm);			               // set pwm duty cycle to pwm value
      }
      break;
    case MODE_MPPT_BAT_FLOAT:
      if (sol_watts < MIN_SOL_WATTS) {         //if watts input from the solar panel is less than
        power_status.mode = MODE_MPPT_OFF;     //the minimum solar watts then it is getting dark so
        off_count = OFF_NUM;                   //go to the charger off state
        set_pwm_duty(pwm);					
        TURN_OFF_MOSFETS; 
      }
      else if (bat_volts > MAX_BAT_VOLTS) {    //since we're in the battery float state if the battery voltage
        pwm -= 1;                              //is above the float voltage back off the pwm to lower it   
        set_pwm_duty(pwm);					
      }
      else if (bat_volts < MAX_BAT_VOLTS) {    //else if the battery voltage is less than the float voltage
        pwm += 1;                              //increment the pwm to get it back up to the float voltage
        set_pwm_duty(pwm);					
        if (pwm >= 100) {                      //if pwm gets up to 100 it means we can't keep the battery at
          power_status.mode = MODE_MPPT_BULK;  //float voltage so jump to charger bulk state to charge the battery
        }
      }
      break;
    case MODE_MPPT_OFF:                           //when we jump into the charger off state, off_count is set with OFF_NUM
      if (off_count > 0) {                        //this means that we run through the off state OFF_NUM of times with out doing
        off_count--;                              //anything, this is to allow the battery voltage to settle down to see if the  
      }                                           //battery has been disconnected
      else if ((bat_volts > HIGH_BAT_VOLTS) && (bat_volts < MAX_BAT_VOLTS) && (sol_volts > bat_volts)) {
        power_status.mode = MODE_MPPT_BAT_FLOAT;  //if battery voltage is still high and solar volts are high
        set_pwm_duty(pwm);		                    //change charger state to battery float			
        TURN_ON_MOSFETS; 
      }
      else if ((bat_volts > MIN_BAT_VOLTS) && (bat_volts < MAX_BAT_VOLTS) && (sol_volts > bat_volts)) {
        pwm = PWM_START;                        // if battery volts aren't quite so high but we have solar volts
        set_pwm_duty(pwm);			                // greater than battery volts showing it is day light then	
        power_status.mode = MODE_MPPT_ON;       // change charger state to on so we start charging
        TURN_ON_MOSFETS; 
      }
      break;
    default:
      TURN_OFF_MOSFETS;                         //else stay in the off state
      break;
  }
}

//------------------------------------------------------------------------------------------------------
// Modified MPPT code. The code for handling battery or low power is removed
// 
//  On State - this is charger state for solar watts > MIN_SOL_WATTS. This is where we run the 
//      Peak Power Tracking alogorithm. In this state we try and run the maximum amount
//      of current that the solar panels are generating into the other side.
//  Off State - This is state that the device enters when solar watts < MIN_SOL_WATTS. The device goes into this
//      state when it gets dark and there is no more power being generated by the solar panels. The MOSFETs are turned
//      off in this state so that power from the battery doesn't leak back into the solar panel. When the charger off
//      state is first entered all it does is decrement off_count for OFF_NUM times. This is done because if the battery
//      is disconnected (or battery fuse is blown) it takes some time before the battery voltage changes enough so we can tell
//      that the battery is no longer connected. This off_count gives some time for battery voltage to change so we can
//      tell this.
//------------------------------------------------------------------------------------------------------

void MPPT_state_machine(void) {
  
  static int old_sol_watts = 0;
  static int off_count = OFF_NUM;
  static int delta = PWM_INC;                   // variable used to modify pwm duty cycle for the ppt algorithm

  int pwm = power_status.pwm_duty;
  int sol_watts = power_status.sol_watts;
  int bat_volts = power_status.bat_volts;
  int sol_volts = power_status.sol_volts;
  
  switch (power_status.mode) {
    case MODE_MPPT_ON:
      if (sol_watts < MIN_SOL_WATTS) {         //if watts input from the solar panel is less than
        power_status.mode = MODE_MPPT_OFF;     //the minimum solar watts then it is getting dark so
        off_count = OFF_NUM;                   //go to the charger off state
        TURN_OFF_MOSFETS; 
      }
      else {                                   // this is where we do the Peak Power Tracking Maximum Power Point algorithm
        if (old_sol_watts >= sol_watts) {      // if previous watts are greater change the value of
          delta = -delta;                      // delta to make pwm increase or decrease to maximize watts
        }
        pwm += delta;                          // add delta to change PWM duty cycle for PPT algorythm 
        old_sol_watts = sol_watts;             // load old_watts with current watts value for next time
        set_pwm_duty(pwm);                     // set pwm duty cycle to pwm value
      }
      break;
      
    case MODE_MPPT_OFF:                           //when we jump into the charger off state, off_count is set with OFF_NUM
      if (off_count > 0) {                        //this means that we run through the off state OFF_NUM of times with out doing
        off_count--;                              //anything, this is to allow the battery voltage to settle down to see if the  
      }                                           //battery has been disconnected
      else if (sol_volts > (bat_volts + SOL_BAT_VOLT_DIFF)) {
        pwm = PWM_START;                        // if we have solar volts more than SOL_BAT_VOLT_DIFF volts higher than battery
        power_status.mode = MODE_MPPT_ON;       // start pumping the power
        set_pwm_duty(pwm);                      
        TURN_ON_MOSFETS; 
      }
      break;
    default:
      TURN_OFF_MOSFETS;                         //else stay in the off state
      break;
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
  int pwm_delta = 1 + (abs(target_difference) / 25);
  
  // adjust pwm either up or down based on difference sign
  if (target_difference < 0) {
    current_pwm -= pwm_delta;
  } else if (target_difference > 0) {
    current_pwm += pwm_delta;
  }
  
  set_pwm_duty(current_pwm);
}

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

    default:
      break;
  }
}

// State machine for different modes
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
    default: // other cases: call mppt state-machine
      MPPT_state_machine();
      break;
  }
}

void get_serial_command() {
  /* Variables used for parsing and tokenising */
  char cmd[BUFF_MAX]; // char string to store the command
  int val;  // Integer to store value

  // read until reaching '{'
  while (Serial.read() != '{');

  char in_buff[BUFF_MAX]; // Buffer in input
  int end = Serial.readBytesUntil('}', in_buff, BUFF_MAX); // Read command from serial monitor
  in_buff[end] = 0; // null terminate the string

  sscanf(in_buff, "%[^= ] = %d}", cmd, &val); // parse the string

  #define stricmp strcasecmp // strangely, arduino uses a different API

  if(!stricmp(cmd,"P")) {
    state_switch(MODE_CONST_POWER, val);
  }
  else if(!stricmp(cmd,"V")) {
    state_switch(MODE_CONST_VOLT, val);
  }
  else if(!stricmp(cmd,"I")) {
    state_switch(MODE_CONST_CURRENT, val);
  }
  else if(!stricmp(cmd,"PWM")) {
    state_switch(MODE_CONST_DUTY, val);
  }
  else if(!stricmp(cmd,"MPPT") && val) {
    state_switch(MODE_MPPT_ON); 
  }
  else if(!stricmp(cmd,"OFF") && val) {
    state_switch(MODE_OFF);
  }
  else if(!stricmp(cmd,"STATUS") && val) {
    print_data_json();
  }
  else if(!stricmp(cmd,"IDN") && val) {
    print_identity();
  }
  else {
    Serial.print("{\"time\": ");
    Serial.print(seconds, DEC);
    Serial.println(", \"state\": \"read_err\"}");
  }
}

//------------------------------------------------------------------------------------------------------
// This routine is automatically called at powerup/reset
//------------------------------------------------------------------------------------------------------
void setup()                               // run once, when the sketch starts
{
  Serial.begin(115200);                    // open the serial port at 9600 bps:
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
    get_serial_command();              // read commands from serial
  }

  /*
  // update power info
  static int current_target = 0;
  static unsigned int prev_seconds = 0;        // seconds value from previous pass
  if ((seconds - prev_seconds) > 4) {  
    prev_seconds = seconds;		// do this stuff once a second

    if (current_target >= 400) {
      current_target = 0;
    }
    else {
      current_target = 400;
    }

    state_switch(MODE_CONST_POWER, current_target);
    //state_switch(MODE_CONST_VOLT, current_target);
  }*/

/*  
  // diagnosistic prints
  static int print_counter = 0;
  print_counter++;
  if (print_counter > 50) {
    print_data_json(); 
    print_counter = 0;
  }
*/  
}
//------------------------------------------------------------------------------------------------------
