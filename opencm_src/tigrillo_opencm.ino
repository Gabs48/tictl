/* Tigrillo_openCM
 
 This script implements the basic functions to interface sensors and actuators over the UART 
 so that they can be used on another board. No major computation or logi is done here.
 
 Created on September 11th 2017
 
 Gabriel Urbain <gabriel.urbain@ugent.be>
 Copyright 2017, Human Brain Projet, SP10
 */

/* -------- Global Variables and Preprocessor Commands -------- */

// Includes
#include <errno.h>

// Defines
#define UART_3 3
#define TIMER_1 1
#define BAUDRATE_1MB 3
#define MAX_ARG_SIZE 100
#define MIN_SENS_READ_TIME 2000
#define DEF_SENS_READ_TIME 1000000

#define ACT_NUM 4
#define SENS_NUM 4
#define SENS_PIN_FL 0
#define SENS_PIN_FR 1
#define SENS_PIN_BL 2
#define SENS_PIN_BR 3
#define ACT_ID_FL 0
#define ACT_ID_FR 1
#define ACT_ID_BL 2
#define ACT_ID_BR 3

#define GOAL_POS_SLACK 30

// Declare library objects
Dynamixel Dxl(UART_3);
HardwareTimer Timer(TIMER_1);

// Sensors variables
long sens_period = DEF_SENS_READ_TIME;
long sens_timestamp = 0;
int sens_foot_zero[SENS_NUM];
int sens_val[SENS_NUM];

// Actuators variables
long act_timestamp = 0;

/* -------- OpenCM Required Functions -------- */

void setup() {

  // Setup Dynamixel protocol
  Dxl.begin(BAUDRATE_1MB);
  Dxl.jointMode(ACT_ID_FL);
  Dxl.jointMode(ACT_ID_FR);
  Dxl.jointMode(ACT_ID_BL);
  Dxl.jointMode(ACT_ID_BR);

  // Setup Timer 1 for periodic sensor reading
  Timer.pause();
  Timer.setPeriod(sens_period);
  Timer.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
  Timer.setCompare(TIMER_CH1, 1);
  Timer.attachInterrupt(TIMER_CH1, readSensors);
  Timer.refresh();
  Timer.resume();

  // Setup Interuption for received UART message
  SerialUSB.begin();
  SerialUSB.attachInterrupt(readUART);

}


void loop() {

  // Nothing to do in the main loop

}

/* -------- Custom Processing Functions -------- */

void readUART(byte* buffer, byte size) {

  // Ensure buffer is clean by setting next address value to 0 (not always true but don't know why)
  buffer[size] = '\0';
  
  // Process received instruction
  char instruction = (char)buffer[0];
  switch (instruction) {
    
    case 'A':
      if (size > 1) {
        int act_leg[MAX_ARG_SIZE];
        int res = str2i((char*)(buffer+1), act_leg, ACT_NUM);
        if (res != 0) {
          if  (res == -1)
            SerialUSB.println("[ACK]: A: Error: Too few numbers in argument list!");
          else if (res == -2)
            SerialUSB.println("[ACK]: A: Error: Too many numbers in argument list!");
          else if (res == -3)
            SerialUSB.println("[ACK]: A: Error: Wrong character in argument list!");
          else if (res == -4)
            SerialUSB.println("[ACK]: A: Error: Unknown!");
          break; 
        }
        updateMotors(act_leg);
        SerialUSB.print("[ACK]: A: Success: ");
        SerialUSB.print(act_leg[0]);
        SerialUSB.print(" ");
        SerialUSB.print(act_leg[1]);
        SerialUSB.print(" ");
        SerialUSB.print(act_leg[2]);
        SerialUSB.print(" ");
        SerialUSB.println(act_leg[3]);
      } 
      else {
        SerialUSB.println("[ACK]: A: Error: Too few arguments!");
      }
      break;
      
    case 'R':
      resetSensors();
      SerialUSB.println("[ACK]: R: Success");
      break;
      
    case 'F':
      if (size > 1) {
        int period[1] = {DEF_SENS_READ_TIME};
        int res1 = str2i((char*)(buffer+1), period, 1);
        if (res1 != 0) {
          if  (res1 == -1)
            SerialUSB.println("[ACK]: F: Error: Too few numbers in argument list!");
          else if (res1 == -2)
            SerialUSB.println("[ACK]: F: Error: Too many numbers in argument list!");
          else if (res1 == -3)
            SerialUSB.println("[ACK]: A: Error: Wrong character in argument list!");
          else if (res1 == -4)
            SerialUSB.println("[ACK]: A: Error: Unknown!");
          break; 
        }
        int res2 = changePeriod(period[0]);
        if (res2 == 0) {
          SerialUSB.print("[ACK]: F: Success: Sensor reading period changed to ");
          SerialUSB.print(period[0]);
          SerialUSB.println(" microseconds!");
        }
        else {
          SerialUSB.print("[ACK]: F: Error: Period is too short. Please, use higher than ");
          SerialUSB.print(MIN_SENS_READ_TIME);
          SerialUSB.println(" microseconds!");
        }
      }
      else {
        SerialUSB.println("[ACK]: F: Error: Too few arguments!");
      }
      break;
      
    default:
      SerialUSB.println("[ACK]: D: Error: Instruction not recognised!");
      break;
  }
  
  // WARNING: Empty buffer after each usage
  emptyStr((char*)buffer, MAX_ARG_SIZE);

}


void readSensors(void) {

  // Get time and check with previous timestamp
  long current_timestamp = micros();

  // Get sensors values
  sens_val[0] = analogRead(SENS_PIN_FL) / 4096.0;
  sens_val[1] = analogRead(SENS_PIN_FR) / 4096.0;
  sens_val[2] = analogRead(SENS_PIN_BL) / 4096.0;
  sens_val[3] = analogRead(SENS_PIN_FR) / 4096.0;

  // Send over USB serial the sensors
  SerialUSB.print("[DATA]: ");
  SerialUSB.print("Sensors values: ");
  SerialUSB.print("Front Left: ");
  SerialUSB.print(sens_val[0]);
  SerialUSB.print("; Front Right: ");
  SerialUSB.print(sens_val[1]);
  SerialUSB.print("; Back Left: ");
  SerialUSB.print(sens_val[2]);
  SerialUSB.print("; Back Right: ");
  SerialUSB.print(sens_val[3]);

  // And timing information
  SerialUSB.print("; Time Stamp: ");
  SerialUSB.print(current_timestamp);
  SerialUSB.print("; Previous Time Stamp: ");
  SerialUSB.print(sens_timestamp);
  sens_timestamp = current_timestamp;
  long operation_time = micros();
  SerialUSB.print("; End of Reading Time Stamp: ");
  SerialUSB.println(operation_time);

}

void resetSensors(void) {

  // Get sensors values and setup as new zeros
  sens_foot_zero[0] = analogRead(SENS_PIN_FL) / 4096.0;
  sens_foot_zero[1] = analogRead(SENS_PIN_FR) / 4096.0;
  sens_foot_zero[2] = analogRead(SENS_PIN_BL) / 4096.0;
  sens_foot_zero[3] = analogRead(SENS_PIN_FR) / 4096.0;

}


void updateMotors(int act_leg[]) {
  
  // Send the new values sequentially to the dynamixel motors
  Dxl.writeWord(ACT_ID_FL, GOAL_POS_SLACK, act_leg[0]);
  Dxl.writeWord(ACT_ID_FR, GOAL_POS_SLACK, act_leg[1]);
  Dxl.writeWord(ACT_ID_BL, GOAL_POS_SLACK, act_leg[2]);
  Dxl.writeWord(ACT_ID_BR, GOAL_POS_SLACK, act_leg[3]);

}


int changePeriod(int period) {

   // Pause timer and change timer frequency
  if (period > MIN_SENS_READ_TIME) {  
    sens_period = period;
    Timer.pause();
    Timer.setPeriod(sens_period);
    Timer.refresh();
    Timer.resume();
    return 0;
  } 
  else {
    return -1;
  }

}

/* -------- Custom Utils Functions -------- */

void printTab(int* tab, int size) {
   
  // Only for debugging purpose
  SerialUSB.print("[ ");
  for (int i = 0; i < size; i++) {
    SerialUSB.print(tab[i]);
    SerialUSB.print(" ");
  }
  SerialUSB.println("]");
  
}


void initTab(int* tab, int size) {
  for (int i = 0; i < size; i++) {
    tab[i] = 0;
  }
}


void emptyStr(char* tab, int size) {
  for (int i = 0; i < size; i++) {
    tab[i] = '\0';
  }
}


int str2i(char* str, int* tab, int size) {
  
  char *end = str;
  int index = 0;

  // Split srting with symbol "," and convert to integer value 
  while(*end) {
    errno = 0;
    int n = strtol(str, &end, 10);
    if (str == end)
        return -3;
    else if (errno != 0 && n == 0)
        return -4;
    
    tab[index] = n;
    while (*end == ',') {
      end++;
    }
    str = end;
    index ++;
  }
  
  // Check the number of integer in the array
  if (index < size) {
    return -1;
  } else if (index > size) {
    return -2;
  } else {
    return 0;
  }
}


