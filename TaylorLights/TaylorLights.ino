/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  This example code is in the public domain.
 */


#include <Wire.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Time.h>


#include "eepromMenu.h"
ESP8266_EEPROM_Configs g_configs;   // From eepromMenu.h
WiFiClient espClient;



int led = LED_BUILTIN;

#define RED_LED_PIN D7
#define AMBER_LED_PIN D6
#define GREEN_LED_PIN D5
#define ARCADE_LED_PIN D2
#define ARCADE_BTN_PIN D3

enum SleepState {
  AWAKE,
  GOING_DOWN,
  SLEEPING,
};

SleepState currSleepState = AWAKE;
unsigned long lastButtonPress = millis();
#define AMBER_TIMEOUT 5000

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const long utcOffsetInSeconds = -28800;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


bool g_light_on = false;

// ** Flip the LED state ******************************************
void toggle_light() {
  if( g_light_on ) {
    digitalWrite(BUILTIN_LED, HIGH);
    g_light_on = false;
  } else {
    digitalWrite(BUILTIN_LED, LOW);
    g_light_on = true;
  }
}

// ** Setup, initialize, and connect to the wifi network ********************************
void wifi_connect(char* ssid, char* password) {
  Serial.print("# Initializing Wifi -> ");
  WiFi.mode(WIFI_STA);                        // Required for wireless to wireless MQTT (for some reason).
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(250);
    toggle_light();
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
}




// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(115200);             // Init serial
  delay(500);                       // Serial settle time

  g_configs.init();
  wifi_connect(g_configs.get_SSID(), g_configs.get_Password());   // Connect to WiFi
    
  // initialize the digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // D1 is reverse
  
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(AMBER_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(ARCADE_LED_PIN, OUTPUT);
  pinMode(ARCADE_BTN_PIN, INPUT_PULLUP);

  currSleepState = AWAKE;
  lastButtonPress = millis();

  timeClient.begin();                 // Start NTP client

  digitalWrite(ARCADE_LED_PIN, HIGH);
  delay(1000);
  digitalWrite(ARCADE_LED_PIN, LOW);
}

// the loop routine runs over and over again forever:
void loop() {
  timeClient.update();
  
  digitalWrite(RED_LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(AMBER_LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(GREEN_LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)

  switch(currSleepState) {
    case AWAKE:
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(AMBER_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, HIGH);
      break;
    case GOING_DOWN:
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(AMBER_LED_PIN, HIGH);
      digitalWrite(GREEN_LED_PIN, LOW);
      if( lastButtonPress + AMBER_TIMEOUT < millis() ) {
        currSleepState = SLEEPING;
      }
      break;
    case SLEEPING:
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(AMBER_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, LOW);
      break;
  }

  if(digitalRead(ARCADE_BTN_PIN) == LOW && currSleepState == AWAKE) {
    lastButtonPress = millis();
    currSleepState = GOING_DOWN;
  }

  if(digitalRead(ARCADE_BTN_PIN) == LOW) {
    digitalWrite(ARCADE_LED_PIN, HIGH);
  } else {
    digitalWrite(ARCADE_LED_PIN, LOW);
  }

  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  //Serial.println(timeClient.getFormattedTime());

  unsigned long currEpoch = timeClient.getEpochTime();
  Serial.println("Curr Epoch: " + String(currEpoch));

  time_t utcCalc = currEpoch;
  Serial.println( day(utcCalc )) ;
Serial.println( hour(utcCalc )) ;
Serial.println( minute(utcCalc ) );
  Serial.println( day(utcCalc )) ;
Serial.println( month(utcCalc )) ;
Serial.println( year(utcCalc ) );

  delay(100);
  
}
