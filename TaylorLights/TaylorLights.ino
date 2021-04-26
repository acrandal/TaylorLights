/**
 * 
 * 
 * 
 * 
 * 
 * 
 */

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Time.h>

#include <Timezone_Generic.h>    // https://github.com/khoih-prog/Timezone_Generic
TimeChangeRule usPDT = {"PDT", Second, Sun, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, Sun, Nov, 2, -480};
Timezone usPT(usPDT, usPST);

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
const long utcOffsetInSeconds = 0;    // TZ calculated with Timezone_Generic library

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Light reset materials
int lastButtonState = HIGH;
#define RESET_PRESS_MIN_COUNT 5
unsigned long pressTimes[RESET_PRESS_MIN_COUNT];
#define RESET_PRESS_TIMEOUT 5000


bool g_light_on = false;

// ** Flip the LED state ******************************************
void toggle_light() {
  if( g_light_on ) {
    digitalWrite(LED_BUILTIN, HIGH);
    g_light_on = false;
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    g_light_on = true;
  }
}


// given a Timezone object, UTC and a string description, convert and print local time with time zone
void printDateTime(Timezone tz, time_t utc, const char *descr)
{
    char buf[40];
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    TimeChangeRule *tcr;        // pointer to the time change rule, use to get the TZ abbrev

    time_t t = tz.toLocal(utc, &tcr);
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
        hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tcr -> abbrev);
    Serial.print(buf);
    Serial.print(' ');
    Serial.println(descr);
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


// ** Once run setup routine ***************************************************************
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

  // Initialize the press counts array to zeros
  for(int i = 0; i < RESET_PRESS_MIN_COUNT; i++) {
    pressTimes[i] = 0;
  }
  lastButtonState = HIGH;

  currSleepState = AWAKE;
  lastButtonPress = millis();

  timeClient.begin();                 // Start NTP client

  digitalWrite(ARCADE_LED_PIN, HIGH);
  delay(1000);
  digitalWrite(ARCADE_LED_PIN, LOW);
}


// ** See if it's time to change back to a green light ***************************************
void checkForWakeUp(Timezone tz, time_t utc) {
  TimeChangeRule *tcr;
  time_t t = tz.toLocal(utc, &tcr);
  int currHour =  hour(t);
  int currMinute = minute(t);

  if(currHour == 6 && currMinute == 30) {
    Serial.println("Time to wake, Taylor!");
    currSleepState = AWAKE;
  }
}


// ** Repeated main loop *********************************************************************
void loop() {
  timeClient.update();
  unsigned long currUTCEpoch = timeClient.getEpochTime();
  
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
      checkForWakeUp(usPT, currUTCEpoch);
      break;
  }

  // Detect a press to enter "going to sleep" state
  if(digitalRead(ARCADE_BTN_PIN) == LOW && currSleepState == AWAKE) {
    lastButtonPress = millis();
    currSleepState = GOING_DOWN;
  }

  // Light up the arcade button pin if the button is pressed
  if(digitalRead(ARCADE_BTN_PIN) == LOW) {
    digitalWrite(ARCADE_LED_PIN, HIGH);
  } else {
    digitalWrite(ARCADE_LED_PIN, LOW);
  }

  // Counting a series of button presses for a reset
  if(lastButtonState == HIGH && digitalRead(ARCADE_BTN_PIN) == LOW) {
    for(int i = 0; i < RESET_PRESS_MIN_COUNT - 1; i++) {
      pressTimes[i] = pressTimes[i + 1];
    }
    pressTimes[RESET_PRESS_MIN_COUNT-1] = millis();
    lastButtonState = LOW;

    Serial.println("Button pressed");

    if(pressTimes[0] + RESET_PRESS_TIMEOUT > millis()) {
      Serial.println("Going awake");
      currSleepState = AWAKE;

      // Zero out button press array to force 5 more presses if done quickly
      for(int i = 0; i < RESET_PRESS_MIN_COUNT; i++) {
        pressTimes[i] = 0;
      }

      // Wait for a button release as to not dive right back into GOING_DOWN
      while(digitalRead(ARCADE_BTN_PIN) == LOW) {
        delay(50);
      }
      delay(250);
    }
    
  } else if( lastButtonState == LOW && digitalRead(ARCADE_BTN_PIN) == HIGH ) {
    lastButtonState = HIGH;
  }

  delay(100);
  
}
