// libraries
#include <SoftwareSerial.h>
#include "TinyGPS++.h"
#include "DFRobot_MICS.h"
#include "TimeLib.h"

DFRobot_MICS_ADC MICS5524(A0, 12);
SoftwareSerial SIM900A(10, 11); // tx, rx
SoftwareSerial NEO6M(4, 3); // tx, rx

// constants
#define CALIBRATION_TIME 0
#define THRESHOLD 250
#define CHAR_NUM 110
#define MOTOR_PIN 9
#define timeOffset 28800 // UTC-8

// variables
char *phoneNumbers[] = {"phone_one", "phone_two", "phone_three"};
char driverName[] = "driver_name";
char latitude[11];
char longitude[11];
char alcoholContent[7];
char Date[] = "00/00";
char Time[] = "00:00";
byte last_second, Second, Minute, Hour, Day, Month;
int Year;
float alcoholLevel = 0;
 
// texts
char nameText[CHAR_NUM], alcoholLevelText[CHAR_NUM], locationText[CHAR_NUM], dateTimeText[CHAR_NUM];

// flags
int smsFlag = 1;
int motorFlag = 1;

// tinygps++ object
TinyGPSPlus gps;

void setup() {
  // serial monitor setup
  Serial.begin(9600);

  // check if serial monitor is available
  if (Serial.available()>0) {
    Serial.println("Serial monitor is available");
  } else {
    Serial.println("Serial monitor is not available!");
  }

  // check if MICS5524 is available
  while (!MICS5524.begin()) {
    Serial.println("MICS5524 is not found!");
    delay(1000);
  } Serial.println("MICS5524 is connected");
  
  // get MICS5524 power mode 
  uint8_t mode = MICS5524.getPowerState();
  if(mode == SLEEP_MODE){
    MICS5524.wakeUpMode();
    Serial.println("MICS5524 wake-up success!");
  }else{
    Serial.println("MICS5524 is in wake-up mode");
  }

  // calibrate mics5524
  while(!MICS5524.warmUpTime(CALIBRATION_TIME)){
    Serial.println("Calibrating MICS5524. Please wait...");
    delay(1000);
  }

  // SIM900A setup
  SIM900A.begin(9600);

  // NEO6M setup
  NEO6M.begin(9600);
  useNEO6M();

  // MOTOR setup
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, HIGH);

  Serial.println();
}

void loop() {
  // get alcohol level from MICS5524
  useMICS5524();
  if (MICS5524.getGasData(C2H5OH) >= THRESHOLD) {
   snprintf(nameText, sizeof(nameText), "Hello, %s attempted to drive under the influence of alcohol.",
           driverName);
   Serial.println("Alcohol above defined threshold!");
  
  // off motor
  // digitalWrite(MOTOR_PIN, LOW);
 
  // send text message
   useSIM900A();
  }

  // get location data
  useNEO6M();
  Serial.println(locationText);
  Serial.println(dateTimeText);
  Serial.println(alcoholLevelText);
  Serial.println();
  delay(1000);
}

void useMICS5524() {
  char gasDataTemp[7];
  float gasData = MICS5524.getGasData(C2H5OH);
  dtostrf(gasData, 5, 2, gasDataTemp);
  changeGasData(gasDataTemp);
  snprintf(alcoholLevelText, sizeof(alcoholLevelText), "Alcohol Level: %s PPM",
           alcoholContent);
}

void changeGasData(const char text[7]) {
  strncpy(alcoholContent, text, sizeof(alcoholContent) - 1);
  longitude[sizeof(alcoholContent) - 1] = '\0';  
}

void useSIM900A() {
  for (int i = 0; i < sizeof(phoneNumbers) / sizeof(phoneNumbers[0]); i++) {
      Serial.println("Sending Message please wait....");
      SIM900A.println("AT+CMGF=1");  
      delay(1000);
      Serial.println("Set SMS Number");
      Serial.println(phoneNumbers[i]);
      SIM900A.print("AT+CMGS=\"");
      SIM900A.print(phoneNumbers[i]);
      SIM900A.println("\"\r");
      delay(1000);
      Serial.println("Set SMS Content");
      SIM900A.println(nameText);
      Serial.println(nameText);
      delay(100);
      SIM900A.println(locationText);
      Serial.println(locationText);
      SIM900A.println(dateTimeText);
      Serial.println(dateTimeText);
      SIM900A.println(alcoholLevelText);
      Serial.println(alcoholLevelText);
      
      delay(100);
      Serial.println("Done");
      SIM900A.println((char)26);
      Serial.println("Message sent succesfully");
  }
}

void changeLatitude(const char text[11]) {
  strncpy(latitude, text, sizeof(latitude) - 1);
  latitude[sizeof(latitude) - 1] = '\0';  // Ensure null-termination
}

void changeLongitude(const char text[11]) {
  strncpy(longitude, text, sizeof(longitude) - 1);
  longitude[sizeof(longitude) - 1] = '\0';  // Ensure null-termination
}

void changeTime() {
  Time[0] = hour() / 10 + '0';
  Time[1] = hour() % 10 + '0';
  Time[3] = minute() / 10 + '0';
  Time[4] = minute() % 10 + '0';
}

void changeDate() {
  Date[0] = Month / 10 + '0';
  Date[1] = Month % 10 + '0';
  Date[3] = Day / 10 + '0';
  Date[4] = Day % 10 + '0';
}

void useNEO6M() {
  char latitudeTemp[11];
  char longitudeTemp[11];
  
  while (NEO6M.available() > 0) {
      gps.encode(NEO6M.read());
      dtostrf(gps.location.lat(), 9, 6, latitudeTemp);
      dtostrf(gps.location.lng(), 9, 6, longitudeTemp);

      if (gps.time.isValid()) {
        Second = gps.time.second();
        Minute = gps.time.minute();
        Hour = gps.time.hour();
      }

      if (gps.date.isValid()) {
        Day = gps.date.day();
        Month = gps.date.month();
        Year = gps.date.year();
      }
      
      if (last_second != gps.time.second()) {
        last_second = gps.time.second();

        // set current UTC time
        setTime(Hour, Minute, Second, Day, Month, Year);

        // add offset to get local time
        adjustTime(timeOffset);
    
        changeLatitude(latitudeTemp);
        changeLongitude(longitudeTemp);
        changeTime();
        changeDate();
      }
      
  }
  snprintf(locationText, sizeof(locationText), "Location: %s,%s",
           latitude, longitude);
  snprintf(dateTimeText, sizeof(dateTimeText), "Date: %s\nTime: %s",
           Date, Time);
}
