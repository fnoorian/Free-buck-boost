#include <stdio.h>
#include <stdlib.h>

#define RELAY_1  6  // White/Green cable, Arduino Digital I/O pin number
#define RELAY_2  7  // Yellow/Black cable, Arduino Digital I/O pin number
#define RELAY_3  8
#define RELAY_4  9

void setup_relays() {
    //set pins as outputs
    pinMode(RELAY_1, OUTPUT);      
    pinMode(RELAY_2, OUTPUT);  
    pinMode(RELAY_3, OUTPUT); 
    pinMode(RELAY_4, OUTPUT);     
}

void set_relay(const int pin, const int val) {
    digitalWrite(pin, val);
}

void print_relay_states_json() {
    Serial.print("{\"Relay1\":");
    Serial.print(digitalRead(RELAY_1));
    Serial.print(", \"Relay2\":");
    Serial.print(digitalRead(RELAY_2));
    Serial.print(", \"Relay3\":");
    Serial.print(digitalRead(RELAY_3));
    Serial.print(", \"Relay4\":");
    Serial.print(digitalRead(RELAY_4));
    Serial.print("}\n");
}

void print_identity() 
{
    Serial.println("{\"device\": \"Relayboard\", \"version\":1}");
}

void get_serial_command() {
  
    /* Variables used for parsing and tokenising */
    char in_buff[16]; // char string to store the command

    while (Serial.read() != '{');   // read until reaching '{'
    int len_read = Serial.readBytesUntil('}', in_buff, sizeof(in_buff)); // Read command from serial monitor
    in_buff[len_read] = 0;  // null terminate the string

    #define stricmp strcasecmp // strangely, arduino uses a different API

    if (len_read > 1) {

        // parse the string
        char cmd[16];
        int val;
        sscanf(in_buff, "\"%[^\"]\": %d}", cmd, &val); // parse the string

        // check for commands
        if (stricmp(cmd, "R1") == 0) { // sets the Relay 1
            set_relay(RELAY_1, val);   
        } 
        else if (stricmp(cmd, "R2") == 0)  {
            set_relay(RELAY_2, val);   // sets the Relay 1
        }
        else if (stricmp(cmd, "R3") == 0)  {
            set_relay(RELAY_3, val);   // sets the Relay 3
        }
        else if (stricmp(cmd, "R4") == 0)  {
            set_relay(RELAY_4, val);   // sets the Relay 4
        }
        else if (stricmp(cmd, "STATUS") ==0) { // read the status
            print_relay_states_json();
        }
        else if (stricmp(cmd, "IDN") ==0) { // Identify board
            print_identity();
        } else {
            Serial.println("{\"state\": \"read_err\"}");
        }
    }
}

void setup() {
    Serial.begin(115200); //begin reading at this baud rate

    setup_relays();

    print_identity();
}

void loop(void) {
    if (Serial.available() >= 4) {
        get_serial_command();
    }
}

