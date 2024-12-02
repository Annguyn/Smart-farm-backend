#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>
#include <AccelStepper.h>
#include <ESP8266mDNS.h> 
#include <Stepper.h>
#define SERVO_PIN1 D0       
#define SERVO_PIN2 D1        
#define RELAY_PUMP_PIN D2    
#define FAN_PWM_PIN D3       
#define FAN_DIR_PIN D4       
#define STEPS_PER_REV 2048   

// AccelStepper curtainStepper(AccelStepper::HALF4WIRE, D5, D6, D7, D8);
Stepper curtainStepper(STEPS_PER_REV, D5, D7, D6, D8);

Servo servo1;
Servo servo2;

const char* ssid = "ASPIRE";
const char* password = "12345678";
const char* host = "anfarm8266"; 

AsyncWebServer server(80);
bool curtainAction = false;
int curtainDirection = 0;  // 1: mở, -1: đóng
int curtainStepsRemaining = 0;
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

  MDNS.begin(host);  
  Serial.println("mDNS responder started");
  Serial.println("Access device via: http://" + String(host) + ".local");

  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(FAN_DIR_PIN, OUTPUT);
  digitalWrite(RELAY_PUMP_PIN, LOW);

  servo1.attach(SERVO_PIN1);
  servo2.attach(SERVO_PIN2);
  curtainStepper.setSpeed(15);
  // curtainStepper.setMaxSpeed(4000.0);      
  // curtainStepper.setAcceleration(100.0); 

  setupEndpoints();

  server.begin();
}

void setupEndpoints() {
  server.on("/curtain/open", HTTP_POST, [](AsyncWebServerRequest *request) {
        curtainAction = true;
        curtainDirection = 1;         
        curtainStepsRemaining = 40960;  
        request->send(200, "text/plain", "Curtain is opening for 100.5 rounds");
    });

    server.on("/curtain/close", HTTP_POST, [](AsyncWebServerRequest *request) {
        curtainAction = true;
        curtainDirection = -1;       
        curtainStepsRemaining = 40960;  
        request->send(200, "text/plain", "Curtain is closing for 100.5 rounds");
    });
  server.on("/fan/set", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("speed", true)) { 
        String speedStr = request->getParam("speed", true)->value();
        int speed = speedStr.toInt();
        speed = constrain(speed, 0, 255);
        analogWrite(FAN_PWM_PIN, speed);
        digitalWrite(FAN_DIR_PIN, HIGH); 
        request->send(200, "text/plain", "Fan speed set to " + String(speed));
        Serial.println("Fan speed set to " + String(speed));
    } else {
        request->send(400, "text/plain", "Missing 'speed' parameter");
        Serial.println("Missing 'speed' parameter");
    }
});


 server.on("/servo/up", HTTP_POST, [](AsyncWebServerRequest *request) {
    servo1Angle += 15;
    servo1.write(servo1Angle);
    request->send(200, "text/plain", "Servo moved up by 15 degrees");
});

server.on("/servo/down", HTTP_POST, [](AsyncWebServerRequest *request) {
    servo1Angle -= 15;
    servo1.write(servo1Angle);
    request->send(200, "text/plain", "Servo moved down by 15 degrees");
});

server.on("/servo/left", HTTP_POST, [](AsyncWebServerRequest *request) {
    servo2Angle -= 15;
    servo2.write(servo2Angle);
    request->send(200, "text/plain", "Servo moved left by 15 degrees");
});

server.on("/servo/right", HTTP_POST, [](AsyncWebServerRequest *request) {
    servo2Angle += 15;
    servo2.write(servo2Angle);
    request->send(200, "text/plain", "Servo moved right by 15 degrees");
});
  server.on("/pump/on", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_PUMP_PIN, HIGH);
    request->send(200, "text/plain", "Pump is ON");
  });

  server.on("/pump/off", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_PUMP_PIN, LOW);
    request->send(200, "text/plain", "Pump is OFF");
  });
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(200, "text/plain", "Hello World");
});
  server.begin();
}

void loop() {
    if (curtainAction && curtainStepsRemaining > 0) {
    long steps = curtainDirection * 512;  
    curtainStepper.step(steps);

    curtainStepsRemaining -= abs(steps); 
    yield();
  }

  if (curtainStepsRemaining <= 0 && curtainAction) {
    curtainAction = false; 
  }


    MDNS.update();
}

void moveCurtainAsync(int direction, long totalSteps) {
    curtainAction = true;     
    curtainDirection = direction;
    curtainStepsRemaining = totalSteps;
}