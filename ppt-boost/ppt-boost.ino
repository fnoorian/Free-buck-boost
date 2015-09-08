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
#define PWM_MAX             0.5*PWM_FULL  // the value for pwm duty cyle 0-100.0% (Resolution in 
#define PWM_MIN             0.01*PWM_FULL // the value for pwm duty cyle 0-100.0%
#define PWM_START           PWM_MIN       // the value for pwm duty cyle 0-100.0%
#define PWM_INC             4             // the value the increment to the pwm value for the ppt algorithm

#define AVG_NUM             2             // number of iterations of the adc routine to average the adc readings
#define SOL_AMPS_SCALE      0.5           // the scaling value for raw adc reading to get solar amps scaled by 100 [(1/(0.005*(3.3k/25))*(5/1023)*100]
#define SOL_VOLTS_SCALE     2.7           // the scaling value for raw adc reading to get solar volts scaled by 100 [((10+2.2)/2.2)*(5/1023)*100]
#define BAT_VOLTS_SCALE     2.7           // the scaling value for raw adc reading to get battery volts scaled by 100 [((10+2.2)/2.2)*(5/1023)*100]

#define ONE_SECOND          50000         // count for number of interrupt in 1 second on interrupt period of 20us
  
#define PIN_LED             13            // LED connected to digital pin 13

//------------------------------------------------------------------------------------------------------
// global variables
struct system_states_t {
   int sol_amps;                         // solar amps in 10 mV (scaled by 100)
   int sol_volts;                        // solar volts in 10 mV (scaled by 100)
   int bat_volts;                        // battery volts in 10 mV (scaled by 100)
   int sol_watts;                        // solar watts in 10 mV (scaled by 100)
   uint16_t pwm_duty;
} power_status;

unsigned int seconds = 0;             // seconds from timer routine
  
void set_pwm_duty(int pwm);

//------------------------------------------------------------------------------------------------------
// This routine is automatically called at powerup/reset
//------------------------------------------------------------------------------------------------------
void setup()                            // run once, when the sketch starts
{
  Serial.begin(115200);                  // open the serial port at 9600 bps:
  pinMode(PWM_ENABLE_PIN, OUTPUT);     // sets the digital pin as output
  Timer1.initialize(20);               // initialize timer1, and set a 20uS period
  Timer1.pwm(PWM_PIN, 0);              // setup pwm on pin 9, 0% duty cycle
  TURN_ON_MOSFETS;                     //turn on MOSFET driver chip
  Timer1.attachInterrupt(callback);    // attaches callback() as a timer overflow interrupt
  
  set_pwm_duty(PWM_START);
}
//------------------------------------------------------------------------------------------------------
// This is interrupt service routine for Timer1 that occurs every 20uS.
// It is only used to incremtent the seconds counter. 
// Timer1 is also used to generate the pwm output.
//------------------------------------------------------------------------------------------------------
void callback()
{
  static unsigned int interrupt_counter = 0;     // counter for 20us interrrupt

  if (interrupt_counter++ > ONE_SECOND) {        //increment interrupt_counter until one second has passed
    interrupt_counter = 0;  
    seconds++;                                   //then increment seconds counter
  }
}
//------------------------------------------------------------------------------------------------------
// This routine reads and averages the analog inputs for this system, solar volts, solar amps and 
// battery volts. It is called with the adc channel number (pin number) and returns the average adc 
// value as an integer. 
//------------------------------------------------------------------------------------------------------
int read_adc(int channel){
  
  int sum = 0;
  
  for (int i=0; i<AVG_NUM; i++) {            // loop through reading raw adc values AVG_NUM number of times  
    int temp = analogRead(channel);          // read the input pin  
    sum += temp;                        // store sum for averaging
   // delayMicroseconds(2);              // pauses for 50 microseconds  
  }
  return(sum / AVG_NUM);                // divide sum by AVG_NUM to get average and return it
}
//-----------------------------------------------------------------------------------
// This function prints int that was scaled by 100 with 2 decimal places
//-----------------------------------------------------------------------------------
void print_int100_dec2(int temp) {

  Serial.print(temp/100,DEC);        // divide by 100 and print interger value
  Serial.print(".");
  if ((temp%100) < 10) {              // if fractional value has only one digit
    Serial.print("0");               // print a "0" to give it two digits
    Serial.print(temp%100,DEC);      // get remainder and print fractional value
  }
  else {
    Serial.print(temp%100,DEC);      // get remainder and print fractional value
  }
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
    Timer1.pwm(PWM_PIN, PWM_FULL - pwm, 20); // use Timer1 routine to set pwm duty cycle at 20uS period
    //Timer1.pwm(PWM_PIN,(PWM_FULL * (long)pwm / 100));
  }												
  else if (pwm == PWM_MAX) {				  // if pwm set to 100% it will be on full but we have 
    Timer1.pwm(PWM_PIN, 4, 1000);             // keep switching so set duty cycle at 99.9% and slow down to 1000uS period 
    //Timer1.pwm(PWM_PIN,(PWM_FULL - 1));              
  }												
}													

//------------------------------------------------------------------------------------------------------
// This routine prints all the data out to the serial port.
//------------------------------------------------------------------------------------------------------
void print_data(void) {
  
  Serial.print(seconds,DEC);
  Serial.print("  ");

  Serial.print("pwm = ");
  print_int100_dec2((long)power_status.pwm_duty * 10000 / (PWM_FULL - 1));
  Serial.print("  ");

  Serial.print("s_amps = ");
  print_int100_dec2(power_status.sol_amps);
  Serial.print("  ");

  Serial.print("s_volts = ");
  //Serial.print(sol_volts,DEC);
  //Serial.print("  ");
  print_int100_dec2(power_status.sol_volts);
  Serial.print("  ");

  Serial.print("s_watts = ");
  print_int100_dec2(power_status.sol_watts);
  Serial.print("  ");

  Serial.print("b_volts = ");
  //Serial.print(bat_volts,DEC);
  print_int100_dec2(power_status.bat_volts);
  Serial.print("  ");

  Serial.print("\n\r");
}
//------------------------------------------------------------------------------------------------------
// This routine reads all the analog input values for the system. Then it multiplies them by the scale
// factor to get actual value in volts or amps. Then it adds on a rounding value before dividing to get
// the result scaled by 100 to give a fractional value of two decimal places. It also calculates the input
// watts from the solar amps times the solar voltage and rounds and scales that by 100 (2 decimal places) also.
//------------------------------------------------------------------------------------------------------
void read_data(void) {
  
  power_status.sol_amps =  SOL_AMPS_SCALE * read_adc(ADC_SOL_AMPS_CHAN) ;// * read_adc(ADC_SOL_AMPS_CHAN); //((read_adc(ADC_SOL_AMPS_CHAN)  * SOL_AMPS_SCALE) + 5) / 10;    //input of solar amps result scaled by 100
  power_status.sol_volts = SOL_VOLTS_SCALE * read_adc(ADC_SOL_VOLTS_CHAN); //((read_adc(ADC_SOL_VOLTS_CHAN) * SOL_VOLTS_SCALE) + 5) / 10;   //input of solar volts result scaled by 100
  power_status.bat_volts = BAT_VOLTS_SCALE * read_adc(ADC_BAT_VOLTS_CHAN); // ((read_adc(ADC_BAT_VOLTS_CHAN) * BAT_VOLTS_SCALE) + 5) / 10;   //input of battery volts result scaled by 100
  power_status.sol_watts = (int)((((long)power_status.sol_amps * (long)power_status.sol_volts) + 50) / 100);    //calculations of solar watts scaled by 10000 divide by 100 to get scaled by 100                 
}
//------------------------------------------------------------------------------------------------------
// Main loop.
// Right now the number of times per second that this main loop runs is set by how long the printing to 
// the serial port takes. You can speed that up by speeding up the baud rate.
// You can also run the commented out code and the charger routines will run once a second.
//------------------------------------------------------------------------------------------------------

void constant_power_control(int target_watts) {
  static int old_sol_watts = 0;
  
  int current_watts = power_status.sol_watts;
  int current_pwm = power_status.pwm_duty;
  
  if (target_watts < current_watts) {
    current_pwm -= PWM_INC;
  } else if (target_watts > current_watts) {
    current_pwm += PWM_INC;
  }
  
  set_pwm_duty(current_pwm);
}

void constant_voltage_control(int target_voltage) {
  static int old_bat_volt = 0;
  
  int current_voltage = power_status.bat_volts;
  int current_pwm = power_status.pwm_duty;
  
  if (target_voltage < current_voltage) {
    current_pwm -= PWM_INC;
  } else if (target_voltage > current_voltage) {
    current_pwm += PWM_INC;
  }
  
  set_pwm_duty(current_pwm);
}

void loop()                          // run over and over again
{
  read_data();                         //read data from inputs
//  MPPT_state_machine();                      //run the charger state machine
  
  static int current_power = 0;
  static unsigned int prev_seconds = 0;        // seconds value from previous pass
  if ((seconds - prev_seconds) > 4) {  
    prev_seconds = seconds;		// do this stuff once a second

    if (current_power >= 1600) {
      current_power = 1200;
    }
    else {
      current_power += 100;
    }
  }

  //constant_voltage_control(current_power);
  constant_voltage_control(current_power);

  static unsigned int print_counter = 0;
  print_counter++;
  if (print_counter > 100) {
    print_data();                       //print data
    print_counter = 0;
  }
  
//    read_data();                         //read data from inputs
//    MPPT_state_machine();
//    print_data();                       //print data
//  }
}
//------------------------------------------------------------------------------------------------------
