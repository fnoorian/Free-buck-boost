#include <stdio.h>
#include <stdlib.h>

#define IN_1  6  // White/Green cable, Arduino Digital I/O pin number
#define IN_2  7  // Yellow/Black cable, Arduino Digital I/O pin number
#define IN_3  8
#define IN_4  9

void setup() {
Serial.begin(9600); //begin reading at this baud rate

//set pins as outputs
pinMode(IN_1, OUTPUT);      
pinMode(IN_2, OUTPUT);  
pinMode(IN_3, OUTPUT); 
pinMode(IN_4, OUTPUT);     
}

void loop() {

   while (Serial.read() != '{');
   
  char buffer[64];              
  int len_read = Serial.readBytesUntil('}', buffer, sizeof(buffer));
  buffer[len_read] = 0;

  #define stricmp strcasecmp // strangely, arduino uses a different API

  if (len_read > 1){

  char cmd[16];
  int val;
  sscanf(buffer, "%[^= ] = %d}", cmd, &val); 

  if (stricmp(cmd, "R1") == 0) {
  digitalWrite(IN_1, val);   // sets the LED on
    
  }

    if (stricmp(cmd, "R2") == 0)  {
  digitalWrite(IN_2, val);   // sets the LED on
    
  }
  
  if (stricmp(cmd, "R3") == 0)  {
  digitalWrite(IN_2, val);   // sets the LED on
    
  }
  
  if (stricmp(cmd, "R4") == 0)  {
  digitalWrite(IN_2, val);   // sets the LED on
    
  }

  if (stricmp(cmd, "ReadRel") ==0) {
  Serial.print("{\"Relay1\":");
  Serial.print(digitalRead(IN_1));
   Serial.print(", \"Relay2\":");
  Serial.print(digitalRead(IN_2));
   Serial.print(", \"Relay3\":");
  Serial.print(digitalRead(IN_3));
   Serial.print(", \"Relay4\":");
  Serial.print(digitalRead(IN_4));
   Serial.print("}\n");
  }
}
}
