#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>
#include <Stepper.h>

#define SERVO_PIN1 D0       
#define SERVO_PIN2 D1        
#define RELAY_PUMP_PIN D2    
#define FAN_PWM_PIN D3       
#define FAN_DIR_PIN D4       
#define STEPS_PER_REV 2048   

Stepper curtainStepper(STEPS_PER_REV, D5, D6, D7, D8);

Servo servo1;
Servo servo2;

const char* ssid = "DOAN VAN ANH";
const char* password = "0979377346";

AsyncWebServer server(80);

int servo1Angle = 90;
int servo2Angle = 90;

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(FAN_DIR_PIN, OUTPUT);
  digitalWrite(RELAY_PUMP_PIN, LOW);

  servo1.attach(SERVO_PIN1);
  servo2.attach(SERVO_PIN2);

  curtainStepper.setSpeed(10);

  setupEndpoints();
}

void setupEndpoints() {
  server.on("/curtain/open", HTTP_GET, [](AsyncWebServerRequest *request) {
    for (int i = 0; i < STEPS_PER_REV; i++) {
      curtainStepper.step(1);
      delay(10);  
    }
    request->send(200, "text/plain", "Curtain is opening");
  });

  server.on("/curtain/close", HTTP_GET, [](AsyncWebServerRequest *request) {
    for (int i = 0; i < STEPS_PER_REV; i++) {
      curtainStepper.step(-1);
      delay(10);
    }
    request->send(200, "text/plain", "Curtain is closing");
  });

  server.on("/fan/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("speed")) {
      int speed = request->getParam("speed")->value().toInt();
      speed = constrain(speed, 0, 255);  
      analogWrite(FAN_PWM_PIN, speed);
      digitalWrite(FAN_DIR_PIN, HIGH);  
      request->send(200, "text/plain", "Fan speed set to " + String(speed));
    } else {
      request->send(400, "text/plain", "Missing 'speed' parameter");
    }
  });

  server.on("/servo/up", HTTP_GET, [](AsyncWebServerRequest *request) {
    servo1Angle = constrain(servo1Angle + 15, 0, 180);
    servo1.write(servo1Angle);
    request->send(200, "text/plain", "Servo moved up by 15 degrees");
  });

  server.on("/servo/down", HTTP_GET, [](AsyncWebServerRequest *request) {
    servo1Angle = constrain(servo1Angle - 15, 0, 180);
    servo1.write(servo1Angle);
    request->send(200, "text/plain", "Servo moved down by 15 degrees");
  });

  server.on("/servo/left", HTTP_GET, [](AsyncWebServerRequest *request) {
    servo2Angle = constrain(servo2Angle - 15, 0, 180);
    servo2.write(servo2Angle);
    request->send(200, "text/plain", "Servo moved left by 15 degrees");
  });

  server.on("/servo/right", HTTP_GET, [](AsyncWebServerRequest *request) {
    servo2Angle = constrain(servo2Angle + 15, 0, 180);
    servo2.write(servo2Angle);
    request->send(200, "text/plain", "Servo moved right by 15 degrees");
  });

  server.on("/pump/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_PUMP_PIN, HIGH);
    request->send(200, "text/plain", "Pump is ON");
  });

  server.on("/pump/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_PUMP_PIN, LOW);
    request->send(200, "text/plain", "Pump is OFF");
  });

  server.begin();
}

void loop() {
  // No need for any code here since the server handles everything
}
