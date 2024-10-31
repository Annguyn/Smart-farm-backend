#include <Servo.h>
#include <Wire.h>
#include <RTClib.h>

#define RELAY_PIN 7 // Connect the relay to pin 7
#define SERVO1_PIN 9 // Connect the first servo to pin 9
#define SERVO2_PIN 10 // Connect the second servo to pin 10

Servo servo1;
Servo servo2;
RTC_DS3231 rtc;

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Pump off by default
    servo1.attach(SERVO1_PIN);
    servo2.attach(SERVO2_PIN);

    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC!");
        while (1);
    }
    // Uncomment the next line to set the RTC time
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set to compile time
}

void loop() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n'); // Read command from ESP8266
        if (command == "PUMP_ON") {
            digitalWrite(RELAY_PIN, HIGH); // Turn pump on
        } else if (command == "PUMP_OFF") {
            digitalWrite(RELAY_PIN, LOW); // Turn pump off
        } else if (command.startsWith("SERVO:")) {
            int servoIndex = command.charAt(6) - '0'; // Get servo number
            int angle = command.substring(8).toInt(); // Get angle
            if (servoIndex == 1) {
                servo1.write(angle); // Control servo 1
            } else if (servoIndex == 2) {
                servo2.write(angle); // Control servo 2
            }
        }
    }


    delay(1000);
}
