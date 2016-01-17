#define BATTERY_MAX_CHARGE_CURRENT        3500            // Battery Max charging rate (3.5 A)
#define BATTERY_TO_OFF_CHARGE_CURRENT     0075            // Battery current to turn off charging (0.75 A)
#define BATTERY_FLOAT_VOLTAGE             1300            // Battery Float voltage (13.0 V)
#define BATTERY_FLOAT_CHARGE_VOLTAGE      1380            // Battery voltage to charge at float mode (13.8 V)

void Charger_switch_mode(const charger_mode_t & mode) {
  switch (mode) {
    case MODE_BATT_OFF:
      power_status.mode = MODE_BATT_OFF;                 // turn off
      power_status.target = 0;
      TURN_OFF_MOSFETS;
      break;
      
    case MODE_BATT_BULK:
      power_status.mode = MODE_BATT_BULK;          // set mode to bulk charging
      power_status.target = BATTERY_MAX_CHARGE_CURRENT;
      TURN_ON_MOSFETS;
      break;

    case MODE_BATT_FLOAT:
      power_status.mode = MODE_BATT_FLOAT;         // set mode to float charging
      power_status.target = BATTERY_MAX_CHARGE_CURRENT;
      TURN_ON_MOSFETS;
      break;
  }
}

void Charger_State_Machine(void) {

  // Get the voltage and current going to the battery
  int batt_voltage = power_status.out_volts;
  int batt_curr = power_status.in_watts / batt_voltage;

  switch (power_status.mode) {
    case MODE_BATT_OFF:
      if (batt_voltage < BATTERY_FLOAT_VOLTAGE) {      // If discharged to float voltage
          Charger_switch_mode(MODE_BATT_BULK);          // set mode to bulk charging
      }
      
      // Else, keep the system as is, that is OFF
      break;
        
    case MODE_BATT_BULK:
      if (batt_voltage >= BATTERY_FLOAT_VOLTAGE) {     // If charged to float voltage
        Charger_switch_mode(MODE_BATT_FLOAT);         // set mode to float charging
      }
      else {                                           // Adjust PWM to get constant current
        adjust_pwm(BATTERY_MAX_CHARGE_CURRENT - batt_curr);
      }
      
      break;
        
    case MODE_BATT_FLOAT:
      if (batt_voltage < BATTERY_FLOAT_VOLTAGE) {            // If discharged to float voltage
        Charger_switch_mode(MODE_BATT_BULK);                 // set mode to bulk charging
      }
      else if (batt_curr < BATTERY_TO_OFF_CHARGE_CURRENT) {  // If fully charged, so not much current drawn
        Charger_switch_mode(MODE_BATT_OFF);                  // turn off
      }
      else {                                                 // Adjust PWM to get constant volrage
        adjust_pwm(BATTERY_FLOAT_CHARGE_VOLTAGE - batt_voltage);
      }
        
      break;
  }
}  
