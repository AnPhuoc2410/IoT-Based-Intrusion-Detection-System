#include <dummy.h>

#define BLYNK_TEMPLATE_ID "TMPL6_iH59a90"
#define BLYNK_TEMPLATE_NAME "IoTHome"
#define BLYNK_AUTH_TOKEN "jWg6M-5ziegBmJTgS3KtdjIwyKl7c-Xt"

#define BLYNK_PRINT Serial
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Your WiFi credentials.
char ssid[] = "FPTU_Student";
char pass[] = "12345678";

void setup()
{
  // Debug console
  Serial.begin(9600);
  Serial.println("Connecting to WiFi and Blynk...");

  // Start the connection to WiFi and Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Blynk.syncAll();
}

void loop()
{
  Blynk.run();
  checkSerial();
}

// Function to handle button presses on Blynk app
BLYNK_WRITE(V1) // Button 1
{
  int state = param.asInt();
  if (state == 1) {
    Serial.println("Sending command 1 to Arduino.");
  } else {
    Serial.println("Stopping command 1.");
  }
}

BLYNK_WRITE(V2) // Button 0
{
  int state = param.asInt();
  if (state == 1) {
    Serial.println("Sending command 0 to Arduino.");
    
  } else {
    Serial.println("Stopping command 0.");
  
  }
}

BLYNK_WRITE(V3) // Button 3
{
  int state = param.asInt();
  if (state == 1) {
    Serial.println("Sending command 3 to Arduino.");
  } else {
    Serial.println("Stopping command 3.");
  }
}

// Function to check serial input from Arduino and update Blynk app
void checkSerial() {
  if (Serial.available()) {
    char command = Serial.read();
    switch (command) {
      case 'a':
        Blynk.virtualWrite(V2, 1); // Update Blynk button to ON
        break;                     // Home LED Button
      case 'b':
        Blynk.virtualWrite(V2, 0); // Update Blynk button to OFF
        break;                     // Home LED Button
      case 'c':
        Blynk.virtualWrite(V1, 1); // Update Blynk button to ON
        break;                     // ALARM Button
      case 'd':
        Blynk.virtualWrite(V1, 0); // Update Blynk button to OFF
        break;                     //ALARM Button
      case 'e':
        Blynk.virtualWrite(V3, 1); // Update Blynk button to ON
        break;                     // All on
      case 'f':
        Blynk.virtualWrite(V3, 0);  // Update Blynk button to OFF
        Blynk.virtualWrite(V1, 0);  // All off
        break;
      case 'w':
        Blynk.logEvent("warning_motion_detected");
        break;
    }
  }
}
