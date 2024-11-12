#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <ESPAsyncWebSrv.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <DFRobotDFPlayerMini.h>
#include <RTClib.h>
#include <IRRemoteESP32.h>  
#include <Adafruit_PN532.h>
#include <HTTPClient.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define MOISTURE_PIN 34
#define SPEAKER_PIN1 33
#define SPEAKER_PIN2 32
#define SPEAKER_POWER_PIN 5 
#define RAIN_PIN 26
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
IRRemoteESP32 irRemote(IR_PIN); 

#define NFC_SCK_PIN 18
#define NFC_MISO_PIN 19
#define NFC_MOSI_PIN 23
#define NFC_SS_PIN 5

Adafruit_PN532 nfc(NFC_SCK_PIN, NFC_MISO_PIN, NFC_MOSI_PIN, NFC_SS_PIN);

HardwareSerial mySerial(1);
const char* esp8266_ip = "192.168.1.1";
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
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  dht.begin();

  lcd.begin(16, 2);
  lcd.print("Smart Garden");

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  mySerial.begin(9600, SERIAL_8N1, 33, 32);
  myDFPlayer.begin(mySerial);
  
  pinMode(SOUND_SENSOR_PIN, INPUT);

  pinMode(RAIN_PIN, INPUT);

  Wire.begin();
  nfc.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // irRemote.begin();

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
    response += "\"motorStatus\": " + String(fanStatus) + ","; 
    response += "\"pumpStatus\": " + String(pumpStatus) + "," ;
    response += "\"distance\": " + String(readDistance()) ;
    response += "}";

    request->send(200, "application/json", response);
  });

  server.on("/motor/on", HTTP_GET, [](AsyncWebServerRequest *request){
    turnOnFan();
    request->send(200, "text/plain", "Motor is ON");
  });

  server.on("/motor/off", HTTP_GET, [](AsyncWebServerRequest *request){
    turnOffFan();
    request->send(200, "text/plain", "Motor is OFF");
  });

  server.on("/pump/on", HTTP_GET, [](AsyncWebServerRequest *request){
    turnOnPump();
    request->send(200, "text/plain", "Water Pump is ON");
  });

  server.on("/pump/off", HTTP_GET, [](AsyncWebServerRequest *request){
    turnOffPump();
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
  server.on("/servo/up", HTTP_GET, [](AsyncWebServerRequest *request){
    moveServoUp();
    request->send(200, "text/plain", "Servo moved up by 15 degrees");
  });

  server.on("/servo/down", HTTP_GET, [](AsyncWebServerRequest *request){
    moveServoDown();
    request->send(200, "text/plain", "Servo moved down by 15 degrees");
  });

  server.on("/servo/left", HTTP_GET, [](AsyncWebServerRequest *request){
    moveServoLeft();
    request->send(200, "text/plain", "Servo moved left by 15 degrees");
  });

  server.on("/servo/right", HTTP_GET, [](AsyncWebServerRequest *request){
    moveServoRight();
    request->send(200, "text/plain", "Servo moved right by 15 degrees");
  });
  server.on("/curtain/open", HTTP_GET, [](AsyncWebServerRequest *request){
    openCurtain(); 
    request->send(200, "text/plain", "Curtain is opening");
  });

  server.on("/curtain/close", HTTP_GET, [](AsyncWebServerRequest *request){
    closeCurtain();
    request->send(200, "text/plain", "Curtain is closing");
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
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/curtain/open"; 

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload); 
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();
  curtainStatus = true;  
}

void closeCurtain() {
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/curtain/close"; 

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString(); 
    Serial.println(payload); 
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();
  curtainStatus = false;  
}

void turnOnPump() {
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/pump/on"; 

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString(); 
    Serial.println(payload); 
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();
  pumpStatus = true;  
}

void turnOffPump() {
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/pump/off"; 
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString(); 
    Serial.println(payload);
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();
  pumpStatus = false;
}

void turnOnFan() {
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/motor/on";

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString(); 
    Serial.println(payload); 
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();
  fanStatus = true;  
}

void turnOffFan() {
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/motor/off";

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();
  fanStatus = false;  
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
  if (action == "low_humidity") {
    myDFPlayer.play(1); 
  } else if (action == "high_humidity") {
    myDFPlayer.play(2);
  } else if (action == "high_temperature") {
    myDFPlayer.play(3);
  } else if (action == "low_temperature") {
    myDFPlayer.play(4);
  } else if (action == "high_temperature") {
    myDFPlayer.play(5);
  } else if (action == "low_temperature") {
    myDFPlayer.play(6);
  } else if (action == "stable") {
    myDFPlayer.play(7);
  } else if (action == "open_fan") {
    myDFPlayer.play(8);
  } else if (action == "off_fan") {
    myDFPlayer.play(9);
  } else if (action == "open_curtain") {
    myDFPlayer.play(10);
  } else if (action == "close_curtain") {
    myDFPlayer.play(11);
  } else if (action == "low_temperature") {
    myDFPlayer.play(12);
  } else if (action == "low_light") {
    digitalWrite(SPEAKER_POWER_PIN, HIGH);  
    myDFPlayer.play(13); 
  } else if (action == "turn_speaker_off") {
    myDFPlayer.stop();
    digitalWrite(SPEAKER_POWER_PIN, LOW);
  }
}
void moveServoUp() {
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/servo/up";

  http.begin(url);  
  int httpCode = http.GET();  
  if (httpCode > 0) {
    String payload = http.getString(); 
    Serial.println(payload);  
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();  
}

void moveServoDown() {
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/servo/down"; 

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();
}

void moveServoLeft() {
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/servo/left";

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();
}

void moveServoRight() {
  HTTPClient http;
  String url = "http://" + String(esp8266_ip) + "/servo/right"; 

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.println("Error in HTTP request");
  }
  http.end();
}

int readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = (duration / 2) / 29.1;  
  return distance;
}