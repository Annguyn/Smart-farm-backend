#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DFRobotDFPlayerMini.h>
#include <Servo.h>
#include <Stepper.h>

#define SERVO_PIN1 D1
#define SERVO_PIN2 D2
#define RELAY_PIN D3        
#define RELAY_FAN_PIN D8   
#define STEPS_PER_REV 2048
Stepper curtainStepper(STEPS_PER_REV, D4, D5, D6, D7); 
Servo servo1;
Servo servo2;

const char* ssid = "Giangvien";
const char* password = "dhbk@2024";
AsyncWebServer server(80);

int servo1Angle = 90;
int servo2Angle = 90;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RELAY_FAN_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  
  digitalWrite(RELAY_FAN_PIN, LOW);  

  servo1.attach(SERVO_PIN1);
  servo2.attach(SERVO_PIN2);
  curtainStepper.setSpeed(10);  
  
  setupEndpoints();
}

void setupEndpoints() {
  server.on("/curtain/open", HTTP_GET, [](AsyncWebServerRequest *request) {
    for (int i = 0; i < STEPS_PER_REV; i++) {
        curtainStepper.step(1);
        delay(5);                
    }
    request->send(200, "text/plain", "Curtain is opening");
  });

  server.on("/curtain/close", HTTP_GET, [](AsyncWebServerRequest *request) {
    for (int i = 0; i < STEPS_PER_REV; i++) {
        curtainStepper.step(-1); 
        delay(5);                 
    }
    request->send(200, "text/plain", "Curtain is closing");
  });


  server.on("/motor/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_FAN_PIN, HIGH);  
    request->send(200, "text/plain", "Motor is ON");
  });

  server.on("/motor/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_FAN_PIN, LOW); 
    request->send(200, "text/plain", "Motor is OFF");
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
    digitalWrite(RELAY_PIN, HIGH);  
    request->send(200, "text/plain", "Pump is ON");
  });

  server.on("/pump/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_PIN, LOW);  
    request->send(200, "text/plain", "Pump is OFF");
  });

  server.begin();
}

void loop() {
  // No need for any code here since the server handles everything
}
