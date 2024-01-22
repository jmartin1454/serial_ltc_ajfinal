#include "SPI.h"

int dac1 = 10;
int dac2 = 9;
int dac3 = 8;
int dac4 = 7;

const int numdacs=4;
int dacs[numdacs]={10,9,8,7};

const int numadc_per_dac=16;

uint8_t chipSelect;
uint8_t channel;

const byte numChars = 400;
char receivedChars[numChars];
char tempChars[numChars];
char messageFromPC[numChars] = {0};
int channelFromPC = 0;
int chipSelectFromPC = 0;
float floatFromPC = 0.0;
boolean newData = false;

const int numchan=64;
float voltages[numchan];
bool goodtransfer=true;

void setup() {
  Serial.begin(9600);
  Serial.println("Enter data in this style <SET chipSelect channel voltage>");

  //delay(1000);
  //Serial.println("Sleep command completed.");

  pinMode(dac1, OUTPUT);
  pinMode(dac2, OUTPUT);
  pinMode(dac3, OUTPUT);
  pinMode(dac4, OUTPUT);
  digitalWrite(dac1, HIGH);
  digitalWrite(dac2, HIGH);
  digitalWrite(dac3, HIGH);
  digitalWrite(dac4, HIGH);

  // assign other pins as outputs as well.
  // Can be useful if other pins are blown out.
  pinMode(6,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(2,OUTPUT);
  digitalWrite(6,HIGH);
  digitalWrite(5,HIGH);
  digitalWrite(4,HIGH);
  digitalWrite(3,HIGH);
  digitalWrite(2,HIGH);
  
  SPI.begin();

  digitalWrite(dac1, LOW);
  SPI.transfer16(0x00e0);
  SPI.transfer16(0x0003);
  digitalWrite(dac1, HIGH);

  digitalWrite(dac2, LOW);
  SPI.transfer16(0x00e0);
  SPI.transfer16(0x0003);
  digitalWrite(dac2, HIGH);

  digitalWrite(dac3, LOW);
  SPI.transfer16(0x00e0);
  SPI.transfer16(0x0003);
  digitalWrite(dac3, HIGH);

  digitalWrite(dac4, LOW);
  SPI.transfer16(0x00e0);
  SPI.transfer16(0x0003);
  digitalWrite(dac4, HIGH);

  reset_voltages();
}


void reset_voltages () {
  for(int i;i<numchan;i++){
    voltages[i]=0.0;
  }
}

void loop() {
  int voltage_index;
  recvWithStartEndMarkers();
  if (newData == true) {
    strcpy(tempChars, receivedChars);
    parseData();
    // showParsedData();

    if (!strncmp(messageFromPC, "SET_ALL", 7)) {
      Serial.println("Setting All");

      if(goodtransfer) {

        for (int j=0;j<numdacs;j++) {

          for (int i=0;i<numadc_per_dac;i++) {

            voltage_index=j*numadc_per_dac+i;
            chipSelect=dacs[j];
            floatFromPC=voltages[voltage_index];

            Serial.print("Setting DAC ");
            Serial.print(chipSelect);
            Serial.print(" channel ");
            Serial.print(i);
            Serial.print(" to ");
            Serial.print(floatFromPC);
            Serial.print(" (voltage_index=");
            Serial.print(voltage_index);
            Serial.println(")");

            digitalWrite(chipSelect,LOW);
            SPI.transfer16(0x0030 | i);
            SPI.transfer16(dac_value(floatFromPC));
            digitalWrite(chipSelect, HIGH);
            delay(10);
          }
        }
      } else {
        Serial.println("Sorry, you did not send enough voltages.");
      }
      reset_voltages();


    } else if (!strncmp(messageFromPC, "SET", 3)) {
      chipSelect = chipSelectFromPC;
      channel = channelFromPC;

      Serial.print("Setting CSbar ");
      Serial.print(chipSelect);
      Serial.print(" channel ");
      Serial.print(channel);
      Serial.print(" to ");
      Serial.print(floatFromPC,7);
      Serial.println(" V");

      digitalWrite(chipSelect, LOW);
      SPI.transfer16(0x0030 | (channel&0xF)); // & channel with 0xF so that only 0-15 can appear -- prevents erroneous commands being sent.
      SPI.transfer16(dac_value(floatFromPC));
      digitalWrite(chipSelect, HIGH);
    }

    newData = false;
  }
}

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      } else {
        receivedChars[ndx] = '\0';
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    } else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void parseData() {
  char *strtokIndx;
  char delimiter[4] = ", ";

  strtokIndx = strtok(tempChars, delimiter);
  strcpy(messageFromPC, strtokIndx);

  Serial.print("The message I received is ");
  Serial.println(messageFromPC);

  goodtransfer=true;

  if (!strncmp(messageFromPC,"SET_ALL",7)) {
    Serial.println("Parsing voltages");
    for (int i=0;i<numchan;i++) {
      strtokIndx = strtok(NULL, delimiter);
      if(strtokIndx==NULL) {
        Serial.println("Error: not enough channels!!!");
        goodtransfer=false;
      }
      floatFromPC = atof(strtokIndx);
      Serial.print("Float ");
      Serial.println(floatFromPC);
      voltages[i]=floatFromPC;
    }
  } else if (!strncmp(messageFromPC,"SET",3)) { // this is a comment
    strtokIndx = strtok(NULL, delimiter);
    chipSelectFromPC = atoi(strtokIndx);
    strtokIndx = strtok(NULL, delimiter);
    channelFromPC = atoi(strtokIndx);
    strtokIndx = strtok(NULL, delimiter);
    floatFromPC = atof(strtokIndx);
  }
}

void showParsedData() {
  Serial.print("Message ");
  Serial.println(messageFromPC);
  Serial.print("CS_Integer ");
  Serial.println(chipSelectFromPC);
  Serial.print("Integer ");
  Serial.println(channelFromPC);
  Serial.print("Float ");
  Serial.println(floatFromPC);
}

uint16_t dac_value(float volts) {
  float minv = -10.0;
  float maxv = 10.0;
  return (uint16_t)((volts - minv) / (maxv - minv) * 65535);
}
