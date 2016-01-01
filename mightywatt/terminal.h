void print_mili_values(unsigned long value) 
{
  unsigned long value_high = value / 1000;
  int value_low = value % 1000;

  Serial.print(value_high);
  Serial.print('.');
  if (value_low <= 99)  Serial.print('0');
  if (value_low <= 9)   Serial.print('0');
  Serial.print(value_low);
}

void sendMessage(byte command) // procedure called when there is a send (read from Arduino) data command; MSB first
{  
  switch (command)
  {
  case IDN:
  case QDC:
    {
      unsigned long maxIdac = IDAC_SLOPE;
      maxIdac = ((4095 * maxIdac) >> 12) + IDAC_INTERCEPT;      
      unsigned long maxIadc = IADC_SLOPE;
      maxIadc = ((4080 * maxIadc) >> 12) + IADC_INTERCEPT;      
      unsigned long maxVdac = VDAC0_SLOPE;
      maxVdac = ((4095 * maxVdac) >> 12) + VDAC0_INTERCEPT;          
      unsigned long maxVadc = VADC0_SLOPE;
      maxVadc = ((4080 * maxVadc) >> 12) + VADC0_INTERCEPT;    

      Serial.print("{\"device\": \"MightyWatt\", \"version\":1,");
      Serial.print("\"FW_VERSION\": \"");
      Serial.print(FW_VERSION);
      Serial.print("\", \"BOARD_REVISION\": \"");
      Serial.print(BOARD_REVISION);            
      Serial.print("\", \"maxIdac\": ");
      print_mili_values(maxIdac);
      Serial.print(", \"maxIadc\": ");
      print_mili_values(maxIadc);
      Serial.print(", \"maxVdac\": ");
      print_mili_values(maxVdac);
      Serial.print(", \"maxVadc\": ");
      print_mili_values(maxVadc);
      Serial.print(", \"MAX_POWER\": ");
      print_mili_values(MAX_POWER);
      Serial.print(", \"DVM_INPUT_RESISTANCE\": ");
      print_mili_values(DVM_INPUT_RESISTANCE);
      Serial.print(", \"TEMPERATURE_THRESHOLD\": ");
      Serial.print(TEMPERATURE_THRESHOLD);           
      Serial.println("}");
      break;
    }
  default:
    {
      Serial.print("{\"current\": ");
      print_mili_values(current);
      Serial.print(", \"voltage\": ");
      print_mili_values(voltage);
      Serial.print(", \"temperature\": ");
      Serial.print(temperature);
      Serial.print(", \"remoteStatus\": ");
      Serial.print(remoteStatus);

      Serial.print(", \"loadStatus\": [");

      if (loadStatus == READY) {
        Serial.print("\"READY\"");
      } else {
          if (loadStatus & CURRENT_OVERLOAD) {
            Serial.print("\"CURRENT_OVERLOAD\"");
            if (loadStatus & ~CURRENT_OVERLOAD) Serial.print(",");
          }
          if (loadStatus & VOLTAGE_OVERLOAD) {
            Serial.print("\"VOLTAGE_OVERLOAD\"");
            if (loadStatus & ~(CURRENT_OVERLOAD | VOLTAGE_OVERLOAD)) Serial.print(",");
          }
          if (loadStatus & POWER_OVERLOAD) {
            Serial.print("\"POWER_OVERLOAD\"");
            if (loadStatus & ~(CURRENT_OVERLOAD | VOLTAGE_OVERLOAD | POWER_OVERLOAD)) Serial.print(",");
          }
          if (loadStatus & OVERHEAT) {
            Serial.print("\"OVERHEAT\"");
          }
      }
      Serial.println("]}");

      loadStatus = READY;
      break;
    }
  }
}

void serve_command(const char * in_buff)
{
  // TODO: cleanup this code, by directly calling setLoad and sendMessage

  #define stricmp strcasecmp

  char cmd[BUFF_MAX]; // char array to store command
  unsigned int val; // Value in mA, mV, mW, or mOhm 
  sscanf(in_buff, "\"%[^\"]\": %d}", cmd, &val); // parse the string

  // Handle set commands
  if(!stricmp(cmd,"I")){
      commandByte = 0b11000000;         // Change commandbyte to that of a set command 
  }else if(!stricmp(cmd,"V")){
      commandByte = 0b11000001;
  }else if(!stricmp(cmd,"P")){
      commandByte = 0b11100010;
  }else if(!stricmp(cmd,"R")){
      commandByte = 0b11100011;
  }else if(!stricmp(cmd,"QDC")){
      commandByte = 0b00011110;
  }else if(!stricmp(cmd,"IDN")){
      commandByte = 0b00011111;
  }else if(!stricmp(cmd,"MR")){
      commandByte = 0b00000000;
  }else if(!stricmp(cmd,"STATUS")){ // Equal to MR
      commandByte = 0b00000000;
  }else{                              // If command is invalid, print error message and return to calling function
    Serial.println("{\"state\": \"read_err\"}\n");
    //Serial.flush();
    return;
  }
    
  if (commandByte & 0b10000000)
  {        
      // set command (write to Arduino)
      setLoad(commandByte & 0b00011111, val);
      // sendMessage(0); // This line is disabled to avoid sending instant replies
  }
  else
  {
      sendMessage(commandByte & 0b00011111);
  }
}

//------------------------------------------------------------------------------------------------------
// Read commands from the serial port
//------------------------------------------------------------------------------------------------------
void serialMonitor() {

  if (Serial.available() > 0)  
  {
    // read until reaching '{'
    while (Serial.read() != '{');

    char in_buff[BUFF_MAX]; // Buffer in input
    int end = Serial.readBytesUntil('}', in_buff, BUFF_MAX); // Read command from serial monitor
    in_buff[end] = 0; // null terminate the string

    serve_command(in_buff);
  }
}
