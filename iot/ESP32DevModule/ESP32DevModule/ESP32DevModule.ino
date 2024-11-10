#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <ESPAsyncWebSrv.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <DFRobotDFPlayerMini.h>
#include <RTClib.h>
#include <IRRemoteESP32.h>  // Correct IR library for ESP32
#include <Adafruit_PN532.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define MOISTURE_PIN 34
#define RELAY_PIN 25
#define SPEAKER_PIN1 33
#define SPEAKER_PIN2 32
#define SPEAKER_POWER_PIN 5 
#define MOTOR_PIN1 12
#define MOTOR_PIN2 14
#define SERVO_PIN1 13
#define SERVO_PIN2 14
#define RAIN_PIN 26
#define STEP_PIN 27
#define SOUND_SENSOR_PIN 15
#define IR_PIN 18
#define LIGHT_SENSOR_PIN A0
#define SDA_PIN 21
#define SCL_PIN 22
#define ECHO_PIN 16
#define TRIG_PIN 17

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DFRobotDFPlayerMini myDFPlayer;
RTC_DS3231 rtc;
IRRemoteESP32 irRemote(IR_PIN);  // Use IRRemoteESP32 for ESP32

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

HardwareSerial mySerial(1);

const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";
AsyncWebServer server(80);

int soundCounter = 0;
bool pumpStatus = false;
bool curtainStatus = false;
bool fanStatus = false;
bool automaticPump = false;
bool speakerStatus = false; 

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  dht.begin();

  lcd.begin(16, 2);
  lcd.print("Smart Garden");

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  mySerial.begin(9600, SERIAL_8N1, 33, 32);
  myDFPlayer.begin(mySerial);

  pinMode(RELAY_PIN, OUTPUT);

  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);

  pinMode(SERVO_PIN1, OUTPUT);
  pinMode(SERVO_PIN2, OUTPUT);

  pinMode(SOUND_SENSOR_PIN, INPUT);

  pinMode(RAIN_PIN, INPUT);

  Wire.begin();
  nfc.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // irRemote.begin();  // Initialize IR receiver

  setupEndpoints();
}

void setupEndpoints() {
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String response = "{";
    response += "\"soilMoisture\": " + String(analogRead(MOISTURE_PIN)) + ","; 
    response += "\"temperature\": " + String(dht.readTemperature()) + ","; 
    response += "\"humidity\": " + String(dht.readHumidity()) + ","; 
    response += "\"light\": " + String(readLightLevel()) + ",";  
    response += "\"rainStatus\": " + String(digitalRead(RAIN_PIN)) + ","; 
    response += "\"soundStatus\": " + String(digitalRead(SOUND_SENSOR_PIN)) + ","; 
    response += "\"motorStatus\": " + String(digitalRead(MOTOR_PIN1)) + ","; 
    response += "\"pumpStatus\": " + String(digitalRead(RELAY_PIN)); 
    response += "}";

    request->send(200, "application/json", response);
  });

  server.on("/motor/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(MOTOR_PIN1, HIGH);
    digitalWrite(MOTOR_PIN2, HIGH);
    request->send(200, "text/plain", "Motor is ON");
  });

  server.on("/motor/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(MOTOR_PIN1, LOW);
    digitalWrite(MOTOR_PIN2, LOW);
    request->send(200, "text/plain", "Motor is OFF");
  });

  server.on("/servo", HTTP_GET, [](AsyncWebServerRequest *request){
    String angle = request->getParam("angle")->value();
    int servoAngle = angle.toInt();
    analogWrite(SERVO_PIN1, servoAngle);
    analogWrite(SERVO_PIN2, servoAngle);
    request->send(200, "text/plain", "Servo moved to angle: " + angle);
  });

  server.on("/pump/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(RELAY_PIN, HIGH);
    request->send(200, "text/plain", "Water Pump is ON");
  });

  server.on("/pump/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(RELAY_PIN, LOW);
    request->send(200, "text/plain", "Water Pump is OFF");
  });
  server.on("/speaker/on", HTTP_GET, [](AsyncWebServerRequest *request){
    speakerStatus = true;
    request->send(200, "text/plain", "Speaker is ON");
    playSpeakerNotification("speaker_on");
  });

  server.on("/speaker/off", HTTP_GET, [](AsyncWebServerRequest *request){
    speakerStatus = false;
    request->send(200, "text/plain", "Speaker is OFF");
    playSpeakerNotification("speaker_off");
  });
  server.begin();
}

void loop() {
  if (analogRead(MOISTURE_PIN) > 800) { 
    openCurtain();
  }

  if (dht.readTemperature() > 30) {
    turnOnFan();
  } else {
    turnOffFan();
  }

  if (analogRead(MOISTURE_PIN) < 300) {  
    if (!pumpStatus) {
      turnOnPump();
      automaticPump = true;
    }
  } else {
    if (pumpStatus) {
      turnOffPump();
      automaticPump = false;
    }
  }

  DateTime now = rtc.now();
  if (now.hour() >= 6 && now.hour() <= 18 && readLightLevel() < 100) {
    turnOnLight();
  } else {
    turnOffLight();
  }

  // IR Handling
  int result = irRemote.checkRemote();
  if (result != -1) {
    Serial.println(result);

    // Add custom commands based on your remote
    if (result == 1) { 
      turnOnFan();
    } 
    else if (result == 2) {  
      turnOffFan();
    }
  }

  // Detect sound (1 clap turns on the pump, 2 turns it off, 3 opens the curtain, 4 closes it)
  if (digitalRead(SOUND_SENSOR_PIN) == HIGH) {
    delay(500);  
    soundCounter++;
    if (soundCounter == 1) {
      turnOnPump();
    } else if (soundCounter == 2) {
      turnOffPump();
    } else if (soundCounter == 3) {
      openCurtain();
    } else if (soundCounter == 4) {
      closeCurtain();
    }
    soundCounter = 0;  
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(dht.readTemperature());
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(dht.readHumidity());
  lcd.print(" %");
}

void openCurtain() {
  analogWrite(SERVO_PIN1, 180);
  analogWrite(SERVO_PIN2, 180);
}

void closeCurtain() {
  analogWrite(SERVO_PIN1, 0);
  analogWrite(SERVO_PIN2, 0);
}

void turnOnPump() {
  digitalWrite(RELAY_PIN, HIGH);
  pumpStatus = true;
}

void turnOffPump() {
  digitalWrite(RELAY_PIN, LOW);
  pumpStatus = false;
}

void turnOnFan() {
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, HIGH);
}

void turnOffFan() {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
}

int readLightLevel() {
  return analogRead(LIGHT_SENSOR_PIN);
}

void turnOnLight() {
  // Implement the logic to turn on light
}

void turnOffLight() {
  // Implement the logic to turn off light
}
void playSpeakerNotification(String action) {
  if (action == "low_soil_moisture") {
    myDFPlayer.play(1);
  } else if (action == "high_soil_moisture") {
    myDFPlayer.play(2);
  } else if (action == "high_humidity") {
    myDFPlayer.play(3);
  } else if (action == "high_temperature") {
    myDFPlayer.play(4);
  } else if (action == "turn_speaker_on") {
    digitalWrite(SPEAKER_POWER_PIN, HIGH);  
    myDFPlayer.play(5); 
  } else if (action == "turn_speaker_off") {
    myDFPlayer.stop();
    digitalWrite(SPEAKER_POWER_PIN, LOW);
  }
}
