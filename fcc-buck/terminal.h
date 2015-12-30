// Collection of functions 

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
  
  Serial.print(g_seconds,DEC);
  Serial.print("  ");

  Serial.print("charger = ");
  if (power_status.mode == MODE_OFF)                 Serial.print("off    ");
  else if (power_status.mode == MODE_MPPT_ON)        Serial.print("mppton ");
  else if (power_status.mode == MODE_MPPT_OFF)       Serial.print("mpptoff");
  else if (power_status.mode == MODE_CONST_VOLT)     Serial.print("volt   ");
  else if (power_status.mode == MODE_CONST_CURRENT)  Serial.print("amps   ");
  else if (power_status.mode == MODE_CONST_POWER)    Serial.print("watt   ");
  else if (power_status.mode == MODE_CONST_DUTY)     Serial.print("duty   ");

  Serial.print("  target = ");
  print_int100_dec2(power_status.target);
  
  Serial.print("  pwm = ");
  print_int100_dec2((long)power_status.pwm_duty * 10000 / (PWM_FULL - 1));
  
  Serial.print("  s_amps = ");
  print_int100_dec2(power_status.sol_amps);
  
  Serial.print("  s_volts = ");
  print_int100_dec2(power_status.sol_volts);

  Serial.print("  s_watts = ");
  print_int100_dec2(power_status.sol_watts);
  
  Serial.print("  b_volts = ");
  print_int100_dec2(power_status.bat_volts);
  
  Serial.print("\n\r");
}

void print_data_json(void) {
  Serial.print("{\"time\": ");
  Serial.print(g_seconds, DEC);

  Serial.print(", \"state\": ");
  if (power_status.mode == MODE_OFF)                 Serial.print("\"off\",    ");
  else if (power_status.mode == MODE_MPPT_ON)        Serial.print("\"mppton\", ");
  else if (power_status.mode == MODE_MPPT_OFF)       Serial.print("\"mpptoff\",");
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
// Reading JSON commands from the serial port
//------------------------------------------------------------------------------------------------------
void serve_serial_command() {
  /* Variables used for parsing and tokenising */
  char cmd[BUFF_MAX]; // char string to store the command
  int val;  // Integer to store value

  // read until reaching '{'
  while (Serial.read() != '{');

  char in_buff[BUFF_MAX]; // Buffer in input
  int end = Serial.readBytesUntil('}', in_buff, BUFF_MAX); // Read command from serial monitor
  in_buff[end] = 0; // null terminate the string

  sscanf(in_buff, "\"%[^\"]\": %d}", cmd, &val); // parse the string

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
    Serial.print(g_seconds, DEC);
    Serial.println(", \"state\": \"read_err\"}");
  }
}
