#include <Wire.h>  //For I2C communication
#include <RTClib.h> // For Real-Time Clock(RTC) functionalities.
#include <IRremote.h> //For infrared remote control functionalities.
#include <SoftwareSerial.h> // For software-based serial communication.

#define PIR_PIN 4
#define LED_NHA_PIN 12
#define LED_BAO_DONG_PIN 13
#define BUZZER_PIN 9
#define IR_RECEIVE_PIN 7

RTC_DS1307 rtc;

IRrecv irrecv(IR_RECEIVE_PIN);
decode_results results;

SoftwareSerial mySerial(2, 3); // RX-3, TX-2

bool systemReset = false;
bool ledNhaState = false;
bool chongTromState = false;
bool alarmActive = false;
bool autoChongTrom = false; // Control automatically turns on anti-theft mode
bool tamThoiTatChongTrom = false; // Variable to monitor the status of temporarily turning off anti-theft mode
unsigned long previousMillis = 0;
const long interval = 10000; // 10 seconds
unsigned long alarmStartTime = 0;
const long alarmDuration = 10000; // 10 seconds

bool pirTriggered = false; // Make sure to only send results to Serial once
bool chongTromTriggered = false; // Control prints out anti-theft notices
int lastResetDay = -1; // Variable to track last reset date

void setup() {
  //Initializes serial communication.
  Serial.begin(9600);
  mySerial.begin(9600);

  //Checks RTC availability.
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //Sets up pin modes.
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_NHA_PIN, OUTPUT);
  pinMode(LED_BAO_DONG_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  //Enables IR reception.
  irrecv.enableIRIn();

  //Prints initial date and time.
  DateTime now = rtc.now();
  Serial.print("Initial Date and Time: ");
  printDateTime(now);

  //Prompts user to set date and time.
  Serial.println("Set date and time following the format YYYY/MM/DD HH:MM:SS");
}

void loop() {
  //Continuously updates the current time from the RTC.
  DateTime now = rtc.now();
  unsigned long currentMillis = millis();

  //Reads serial input to set the RTC time.
  if (Serial.available()) {
    String dateTime = Serial.readStringUntil('\n');
    setRTC(dateTime);
  }

  //Reads commands from the SoftwareSerial.
  if (mySerial.available()) {
    char command = mySerial.read();
    handleCommand(command);
  }
  
  //Handles periodic actions based on time intervals.
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.print("Current Date and Time: ");
    printDateTime(now);
  }
  
  //Process IR commands
  if (irrecv.decode(&results)) {
    handleIRCommand(results.value);
    irrecv.resume();
  }

  // Manages LED and alarm based on time and PIR sensor input
  //Between 18:00 and 22:00: Turns on the house LED if motion is detected.
  if (now.hour() >= 18 && now.hour() < 22) {
    if (digitalRead(PIR_PIN) == HIGH && !ledNhaState) {
      digitalWrite(LED_NHA_PIN, HIGH);
      ledNhaState = true;
      mySerial.write('a');
      if (!pirTriggered) {
        Serial.println("PIR detected motion and the house LED was off. Turning on house LED.");
        pirTriggered = true; //Make sure to only send results to Serial once
      }
    }
  } else {
    pirTriggered = false; // Reset variable when out of time frame
  }

  //Between 22:00 and 06:00: Automatically enables the anti-theft mode.
  if ((now.hour() >= 22 || now.hour() < 6) && !autoChongTrom && !chongTromTriggered) {
    chongTromState = true;
    chongTromTriggered = true; // Make sure to only print it out once at 10pm
    Serial.println("Anti-theft mode auto enable");
    mySerial.write('c');
  }

  //Resets the system state at specific times (06:00:01 and 21:59:59). 
  if (now.hour() == 6 && now.minute() == 0 && now.second() == 1 || now.hour() == 21 && now.minute() == 59 && now.second() == 59) {
    autoChongTrom = false; 
    chongTromState = false;
    chongTromTriggered = false; //Reset variable when out of time frame
    tamThoiTatChongTrom = false;
    lastResetDay = now.day(); //update last reset day
    mySerial.write('d');
          if (!systemReset) {
        Serial.println("System reset back to normal");
        systemReset = true; //Make sure to only send results to Serial once
      }
    }
  else {
    systemReset = false; //Reset variable when out of time frame
  }
  

  //Between 22:00 and 06:00: Automatically re-enables the anti-theft mode if turned OFF.
  if ((now.hour() >= 22 || now.hour() < 6) && tamThoiTatChongTrom) {
    chongTromState = true;
    tamThoiTatChongTrom = false;
    mySerial.write('c');
    Serial.println("Auto theft-mode auto re-enable");
  }

  //Activates the buzzer and alarm LED when the PIR sensor detects motion in anti-theft mode.
  if (chongTromState) {
    if (digitalRead(PIR_PIN) == HIGH && !alarmActive) {
      alarmActive = true;
      alarmStartTime = currentMillis;
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(LED_BAO_DONG_PIN, HIGH);
      mySerial.write('w');
      Serial.println("PIR detected motion during anti-theft mode. Triggering alarm.");
    }
  }

  //Deactivates the alarm after a specified duration.
  if (alarmActive) {
    if (currentMillis - alarmStartTime >= alarmDuration) {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_BAO_DONG_PIN, LOW);
      alarmActive = false;
      Serial.println("Alarm cycle complete.");
    }
  }
}

void handleCommand(char command) {
  switch (command) {
    case '0': //Toggles the house LED.
      ledNhaState = !ledNhaState;
      digitalWrite(LED_NHA_PIN, ledNhaState ? HIGH : LOW);
      mySerial.write(ledNhaState ? 'a' : 'b'); // Update Blynk
      Serial.println(ledNhaState ? "Turning on house LED via remote." : "Turning off house LED via remote.");
      break;
    case '1': //Toggles the anti-theft mode.
      if (chongTromState) {
        chongTromState = false;
        tamThoiTatChongTrom = true; // Temporarily turn off anti-theft and enable it to turn back on automatically
        mySerial.write('d'); // Update Blynk
        Serial.println("Anti-theft mode OFF");
      } else {
        chongTromState = true;
        tamThoiTatChongTrom = false; // When turned back on, cancels the temporary off state
        mySerial.write('c'); // Update Blynk
        Serial.println("Anti-theft mode ON");
      }
      alarmActive = false;
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_BAO_DONG_PIN, LOW);
      break;
    case '3': //Disables all alarms and the anti-theft mode.
      chongTromState = false;
      autoChongTrom = false;
      tamThoiTatChongTrom = false; // Cancel the temporary off state when completely turned off
      alarmActive = false;
      digitalWrite(LED_BAO_DONG_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      mySerial.write('f'); // Update Blynk
      Serial.println("Anti-theft mode OFF");
      break;
  }
}

//Processes specific IR commands to control the house LED and anti-theft mode.
void handleIRCommand(unsigned long command) {
  switch (command) {
    case 0xFF6897: // Button 0 (Turn ON/OFF house LED)
      handleCommand('0');
      break;
    case 0xFF30CF: // Button 1 (Turn ON/OFF Anti-theft mode)
      handleCommand('1');
      break;
    case 0xFF7A85: // Button 3 (Turn OFF Anti-theft mode)
      handleCommand('3');
      break;
  }
}

//Sets the RTC based on user input.
void setRTC(String input) {
  int year, month, day, hour, minute, second;
  if (sscanf(input.c_str(), "%d/%d/%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    Serial.print("RTC time has been set to: ");
    Serial.print(year);
    Serial.print('/');
    Serial.print(month);
    Serial.print('/');
    Serial.print(day);
    Serial.print(" ");
    Serial.print(hour);
    Serial.print(':');
    Serial.print(minute);
    Serial.print(':');
    Serial.println(second);
  } else {
    Serial.println("");
  }
}

//Prints the current date and time.
void printDateTime(DateTime now) {
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
}
