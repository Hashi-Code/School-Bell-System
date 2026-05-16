// ============================================================
//  AUTOMATIC SCHOOL BELL SYSTEM
//  ESP32 + DS3231 RTC + CD4017 Counter + Bluetooth App
// ============================================================
//
//  BEFORE YOU UPLOAD THIS CODE, DO THESE 3 THINGS IN ARDUINO IDE:
//
//  1. Install ESP32 board support:
//     - Go to File > Preferences
//     - In "Additional Board URLs" paste this:
//       https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
//     - Go to Tools > Board > Boards Manager
//     - Search "esp32" and install "esp32 by Espressif Systems"
//
//  2. Install RTClib (for DS3231 clock module):
//     - Go to Tools > Manage Libraries
//     - Search "RTClib" and install "RTClib by Adafruit"
//
//  3. Select your board before uploading:
//     - Go to Tools > Board > esp32 > "ESP32 Dev Module"
//     - Go to Tools > Port > select the COM port your ESP32 is on
//       (it appears when you plug in the USB cable)
//
// ============================================================
//  WIRING GUIDE (what connects to what)
// ============================================================
//
//  DS3231 RTC MODULE:
//    VCC  --> ESP32 3.3V
//    GND  --> ESP32 GND
//    SDA  --> ESP32 GPIO 21
//    SCL  --> ESP32 GPIO 22
//
//  CD4017 DECADE COUNTER:
//    Pin 16 (VSS/GND) --> GND
//    Pin 8  (VDD)     --> 3.3V
//    Pin 15 (Reset)   --> ESP32 GPIO 26  (we reset it manually)
//    Pin 14 (Clock)   --> ESP32 GPIO 27  (we pulse this to count up)
//    Pin 3  (Q0)      --> LED for Period 1  (optional visual)
//    Pin 2  (Q1)      --> LED for Period 2  (optional visual)
//    ... and so on up to Q7 for 8 periods
//
//  ACTIVE BUZZER:
//    +  --> ESP32 GPIO 18
//    -  --> GND
//
//  PUSH BUTTON (manual bell):
//    One side --> ESP32 GPIO 19
//    Other    --> GND
//    (ESP32 uses internal pull-up, so no extra resistor needed)
//
// ============================================================

// --- Libraries ---
#include "BluetoothSerial.h"   // built-in, lets ESP32 talk to your phone
#include <Wire.h>              // built-in, needed for I2C (DS3231 uses I2C)
#include <RTClib.h>            // Adafruit RTClib, for reading the DS3231 clock

// --- Pin definitions ---
#define BUZZER_PIN     18   // buzzer connected here
#define CD4017_CLOCK   27   // each pulse here = counter goes up by 1 period
#define CD4017_RESET   26   // pulling this HIGH resets counter back to period 1
#define MANUAL_BUTTON  19   // push button for manual bell ring

// --- Objects ---
BluetoothSerial SerialBT;   // this is our Bluetooth connection object
RTC_DS3231 rtc;             // this is our clock module object

// --- Bell schedule storage ---
// We store up to 20 bell times as "hour * 60 + minute" (minutes since midnight)
// Example: 8:30 AM = 8*60 + 30 = 510
int bellSchedule[20];       // array to hold bell times
int bellCount = 0;          // how many bells are currently scheduled
bool bellFiredToday[20];    // tracks if each bell has already rung today
                            // (prevents ringing again if ESP32 checks same minute twice)

// --- State variables ---
int currentPeriod = 0;      // which period we're on (0 = not started)
bool systemActive = true;   // can be turned off from app

// ============================================================
//  SETUP — runs once when ESP32 powers on
// ============================================================
void setup() {
  Serial.begin(115200);     // for debugging — open Serial Monitor in Arduino IDE to see messages

  // Set pin modes
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CD4017_CLOCK, OUTPUT);
  pinMode(CD4017_RESET, OUTPUT);
  pinMode(MANUAL_BUTTON, INPUT_PULLUP);  // INPUT_PULLUP means button reads HIGH normally, LOW when pressed

  // Start Bluetooth with the name your phone will see
  SerialBT.begin("SchoolBell");
  Serial.println("Bluetooth started — device name: SchoolBell");

  // Start the RTC clock module
  if (!rtc.begin()) {
    Serial.println("ERROR: DS3231 not found! Check your wiring.");
    while (1);  // stop here if clock not found
  }

  // If RTC lost power and reset, set it to your computer's current time
  // This only runs once when you first upload — after that DS3231 keeps time on its own battery
  if (rtc.lostPower()) {
    Serial.println("RTC lost power — setting time to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Reset CD4017 counter to period 1 at startup
  resetCounter();

  Serial.println("System ready!");
}

// ============================================================
//  LOOP — runs forever, over and over
// ============================================================
void loop() {

  // --- 1. Check for Bluetooth commands from the app ---
  handleBluetoothCommands();

  // --- 2. Check if it's time for a bell ---
  if (systemActive) {
    checkBellSchedule();
  }

  // --- 3. Check manual push button ---
  if (digitalRead(MANUAL_BUTTON) == LOW) {  // LOW means button is pressed
    delay(50);                               // small delay to avoid false triggers
    if (digitalRead(MANUAL_BUTTON) == LOW) {
      Serial.println("Manual bell pressed!");
      ringBell();
    }
    while (digitalRead(MANUAL_BUTTON) == LOW); // wait for button release
  }

  delay(500);  // check everything twice per second — saves power, plenty fast enough
}

// ============================================================
//  FUNCTION: Check if current time matches any bell time
// ============================================================
void checkBellSchedule() {
  DateTime now = rtc.now();  // get current time from DS3231
  int currentMinutes = now.hour() * 60 + now.minute();  // convert to minutes since midnight

  // Reset "fired today" flags at midnight
  if (now.hour() == 0 && now.minute() == 0 && now.second() < 2) {
    for (int i = 0; i < bellCount; i++) {
      bellFiredToday[i] = false;
    }
    resetCounter();  // new day = reset period counter
    Serial.println("Midnight reset done");
  }

  // Check each scheduled bell time
  for (int i = 0; i < bellCount; i++) {
    if (currentMinutes == bellSchedule[i] && !bellFiredToday[i]) {
      Serial.print("Bell time matched! Period: ");
      Serial.println(currentPeriod + 1);
      ringBell();
      advancePeriod();        // move CD4017 counter forward by 1
      bellFiredToday[i] = true;  // mark as done so it doesn't ring again this minute
    }
  }
}

// ============================================================
//  FUNCTION: Ring the buzzer for 2 seconds
// ============================================================
void ringBell() {
  Serial.println("RING!");
  digitalWrite(BUZZER_PIN, HIGH);  // buzzer ON
  delay(2000);                     // wait 2 seconds
  digitalWrite(BUZZER_PIN, LOW);   // buzzer OFF
}

// ============================================================
//  FUNCTION: Send one clock pulse to CD4017 (advance period by 1)
// ============================================================
void advancePeriod() {
  currentPeriod++;
  if (currentPeriod > 9) {
    currentPeriod = 0;
    resetCounter();
    return;
  }
  // Send a short HIGH-LOW pulse on the clock pin
  digitalWrite(CD4017_CLOCK, HIGH);
  delay(10);
  digitalWrite(CD4017_CLOCK, LOW);
  Serial.print("Period advanced to: ");
  Serial.println(currentPeriod);
}

// ============================================================
//  FUNCTION: Reset CD4017 back to period 1
// ============================================================
void resetCounter() {
  currentPeriod = 0;
  digitalWrite(CD4017_RESET, HIGH);  // pull reset pin HIGH to reset
  delay(10);
  digitalWrite(CD4017_RESET, LOW);   // bring it back LOW
  Serial.println("Counter reset to period 1");
}

// ============================================================
//  FUNCTION: Handle commands coming from the mobile app
// ============================================================
//
//  COMMAND PROTOCOL (what the app sends):
//
//  "RING"          --> ring the bell immediately
//  "RESET"         --> reset period counter to 1
//  "STOP"          --> disable automatic bells
//  "START"         --> enable automatic bells
//  "ADD:HH:MM"     --> add a bell time e.g. "ADD:08:30" = add 8:30 AM
//  "CLEAR"         --> clear all scheduled bell times
//  "LIST"          --> ESP32 sends back current schedule to app
//  "TIME"          --> ESP32 sends back current time to app
//  "SET:HH:MM:SS"  --> set the RTC time manually e.g. "SET:07:30:00"
//
void handleBluetoothCommands() {
  if (!SerialBT.available()) return;  // nothing received, skip

  String command = SerialBT.readStringUntil('\n');  // read until newline
  command.trim();  // remove any extra spaces or \r characters
  Serial.print("Received: ");
  Serial.println(command);

  // Ring bell manually
  if (command == "RING") {
    ringBell();
    SerialBT.println("OK:Bell rang");
  }

  // Reset counter
  else if (command == "RESET") {
    resetCounter();
    SerialBT.println("OK:Counter reset");
  }

  // Stop automatic bells
  else if (command == "STOP") {
    systemActive = false;
    SerialBT.println("OK:System stopped");
  }

  // Start automatic bells
  else if (command == "START") {
    systemActive = true;
    SerialBT.println("OK:System started");
  }

  // Clear all bell times
  else if (command == "CLEAR") {
    bellCount = 0;
    for (int i = 0; i < 20; i++) bellFiredToday[i] = false;
    SerialBT.println("OK:Schedule cleared");
  }

  // Send current schedule back to app
  else if (command == "LIST") {
    SerialBT.print("SCHEDULE:");
    for (int i = 0; i < bellCount; i++) {
      int h = bellSchedule[i] / 60;
      int m = bellSchedule[i] % 60;
      // format as HH:MM
      if (h < 10) SerialBT.print("0");
      SerialBT.print(h);
      SerialBT.print(":");
      if (m < 10) SerialBT.print("0");
      SerialBT.print(m);
      if (i < bellCount - 1) SerialBT.print(",");
    }
    SerialBT.println();
  }

  // Send current time back to app
  else if (command == "TIME") {
    DateTime now = rtc.now();
    SerialBT.print("TIME:");
    SerialBT.print(now.hour());   SerialBT.print(":");
    if (now.minute() < 10) SerialBT.print("0");
    SerialBT.print(now.minute()); SerialBT.print(":");
    if (now.second() < 10) SerialBT.print("0");
    SerialBT.println(now.second());
  }

  // Add a bell time: format "ADD:HH:MM"
  else if (command.startsWith("ADD:")) {
    if (bellCount >= 20) {
      SerialBT.println("ERROR:Schedule full (max 20)");
      return;
    }
    // Extract hour and minute from "ADD:08:30"
    String timeStr = command.substring(4);  // "08:30"
    int colonPos = timeStr.indexOf(':');
    if (colonPos == -1) {
      SerialBT.println("ERROR:Bad format. Use ADD:HH:MM");
      return;
    }
    int h = timeStr.substring(0, colonPos).toInt();
    int m = timeStr.substring(colonPos + 1).toInt();
    if (h < 0 || h > 23 || m < 0 || m > 59) {
      SerialBT.println("ERROR:Invalid time");
      return;
    }
    bellSchedule[bellCount] = h * 60 + m;
    bellFiredToday[bellCount] = false;
    bellCount++;
    SerialBT.print("OK:Added ");
    SerialBT.print(h); SerialBT.print(":"); 
    if (m < 10) SerialBT.print("0");
    SerialBT.println(m);
  }

  // Set the RTC time: format "SET:HH:MM:SS"
  else if (command.startsWith("SET:")) {
    String timeStr = command.substring(4);  // "07:30:00"
    int c1 = timeStr.indexOf(':');
    int c2 = timeStr.lastIndexOf(':');
    if (c1 == -1 || c2 == -1 || c1 == c2) {
      SerialBT.println("ERROR:Bad format. Use SET:HH:MM:SS");
      return;
    }
    int h = timeStr.substring(0, c1).toInt();
    int m = timeStr.substring(c1 + 1, c2).toInt();
    int s = timeStr.substring(c2 + 1).toInt();
    // Keep today's date, just update the time
    DateTime now = rtc.now();
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, s));
    SerialBT.println("OK:Time updated");
  }

  else {
    SerialBT.println("ERROR:Unknown command");
  }
}
