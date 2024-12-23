#include <Wire.h>
#include <DHT.h>
#include <ESPAsyncWebSrv.h>
#include <WiFi.h>
#include <DFRobotDFPlayerMini.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <ESPmDNS.h>
#include <driver/adc.h> 
#include <AccelStepper.h>
#include "esp_task_wdt.h"

#define DHTPIN 4
#define DHTTYPE DHT11

#define MOISTURE_CHANNEL   ADC1_CHANNEL_6  // GPIO34
#define LIGHT_CHANNEL ADC1_CHANNEL_3  // GPIO39 VN
#define SOUND_CHANNEL      ADC1_CHANNEL_7  // GPIO35
#define WATER_LEVEL_CHANNEL ADC1_CHANNEL_0 // GPIO36 VP

#define SPEAKER_RX_PIN 33 // RX cua DFPLAYERMINI LA 32 
#define SPEAKER_TX_PIN 32
#define ECHO_PIN 16
#define TRIG_PIN 17

//  Động cơ
#define SERVO_PIN1 21
#define SERVO_PIN2 22
#define RELAY_PUMP_PIN 19
#define FAN_PWM_PIN 18
#define STEPS_PER_REV 2048
#define LED_PIN 23 


bool ledStatus = false;
bool automaticLight = false;
int lightThreshold = 2000;

DHT dht(DHTPIN, DHTTYPE);
DFRobotDFPlayerMini myDFPlayer;
HardwareSerial mySerial(1);
AsyncWebServer server(80);

Servo servo1, servo2;

AccelStepper curtainStepper(AccelStepper::FULL4WIRE, 25, 27, 26, 13);
// Stepper curtainStepper(STEPS_PER_REV, 25, 26, 27, 13);

const char *ssid = "C128-1";
const char *password = "sensei_1234";
int curtainDirection = 0; // 1 for open, -1 for close
bool pumpStatus = false, curtainStatus = false;
int fanStatus = 0;
bool automaticPump = false, automaticCurtain = false, automaticFan = false;
int soundCounter = 0;
int servo1Angle = 90 ;
int servo2Angle = 90 ;
bool isCurtainActive = false;
int curtainStepsRemaining = 0;
int soilMoistureThreshold = 2400;
int temperatureHighThreshold = 35;
int temperatureLowThreshold = 20;
int waterLevelThreshold = 2400;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
  void handleFan(int speed, AsyncWebServerRequest *request= nullptr);
  void handlePump(bool status, AsyncWebServerRequest *request = nullptr);
  void handleCurtain(bool status, AsyncWebServerRequest *request = nullptr);

  dht.begin();
  servo1.attach(SERVO_PIN1); 
  servo2.attach(SERVO_PIN2); 
  // curtainStepper.setSpeed(15);
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(FAN_PWM_PIN, OUTPUT);
  digitalWrite(FAN_PWM_PIN,LOW);
  digitalWrite(RELAY_PUMP_PIN,LOW);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  mySerial.begin(9600, SERIAL_8N1, SPEAKER_RX_PIN, SPEAKER_TX_PIN);  // RX = 33, TX = 32
  if (myDFPlayer.begin(mySerial)) {
    Serial.println("DFPlayer Mini initialized successfully.");
    myDFPlayer.volume(30);
  } else {
    Serial.println("DFPlayer Mini initialization failed!");
    Serial.println("1. Kiểm tra kết nối RX/TX.");
    Serial.println("2. Đảm bảo thẻ nhớ microSD đúng định dạng.");
  }
   esp_task_wdt_config_t wdtConfig = {
        .timeout_ms = 10000, 
        .idle_core_mask = 0, 
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdtConfig);
    esp_task_wdt_add(NULL); 
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  if (MDNS.begin("anfarm32")) {
    Serial.println("mDNS responder started at http://anfarm32.local");
  }
  curtainStepper.setMaxSpeed(1000);
  curtainStepper.setAcceleration(200); 
  adc1_config_width(ADC_WIDTH_BIT_12); 
  adc1_config_channel_atten(MOISTURE_CHANNEL, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(LIGHT_CHANNEL, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(SOUND_CHANNEL, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(WATER_LEVEL_CHANNEL, ADC_ATTEN_DB_11); 
  setupEndpoints();
}


void loop() {
  esp_task_wdt_reset(); 
  if (curtainStepsRemaining > 0) {
    curtainStepper.run();
    vTaskDelay(pdMS_TO_TICKS(5));
    if (curtainStepper.distanceToGo() == 0) {
        curtainStepsRemaining -= 512;
    }
    if (curtainStepsRemaining <= 0) {
        curtainStepper.stop();
        curtainStepsRemaining = 0;
    }
}
  handleAutomation();
}

void setupEndpoints() {
    // Endpoint to get data
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String response = "{";
        response += "\"soilMoisture\":" + String(adc1_get_raw(MOISTURE_CHANNEL)) + ",";
        response += "\"temperature\":" + String(dht.readTemperature()) + ",";
        response += "\"humidity\":" + String(dht.readHumidity()) + ",";
        response += "\"light\":" + String(adc1_get_raw(LIGHT_CHANNEL)) + ",";
        response += "\"waterLevel\":" + String(adc1_get_raw(WATER_LEVEL_CHANNEL)) + ",";
        response += "\"soundStatus\":" + String(adc1_get_raw(SOUND_CHANNEL)) + ",";
        response += "\"fanStatus\":" + String(fanStatus) + ",";
        response += "\"pumpStatus\":" + String(pumpStatus) + ",";
        response += "\"curtainStatus\":" + String(curtainStatus) + ",";
        response += "\"distance\":" + String(readDistance()) + ",";
        response += "\"automaticFan\":" + String(automaticFan) + ",";
        response += "\"automaticPump\":" + String(automaticPump) + ",";
        response += "\"ledStatus\":" + String(ledStatus) + "," ;
        response += "\"automaticLight\":" + String(automaticLight) + "," ;
        response += "\"automaticCurtain\":" + String(automaticCurtain) + "}";
        request->send(200, "application/json", response);
    });

    // Fan endpoints
    server.on("/fan/set", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("speed", true)) {
            String speedStr = request->getParam("speed", true)->value();
            int speed = constrain(speedStr.toInt(), 0, 255);
            analogWrite(FAN_PWM_PIN, speed);
            fanStatus = speed;
            playSpeakerNotification("open_fan");
            request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Fan speed set to " + String(speed) + "\"}");
        } else {
            playSpeakerNotification("off_fan");
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'speed' parameter\"}");
        }
    });

    server.on("/dc/automatic", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("status", true)) {
            String status = request->getParam("status", true)->value();
            automaticFan = (status == "on");
            playSpeakerNotification("pump_on");
            request->send(200, "application/json", "{\"status\":\"success\",\"automaticFan\":" + String(automaticFan) + "}");
        } else {
            playSpeakerNotification("stable_conditions");
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'status' parameter\"}");
        }
    });

    // Pump endpoints
    server.on("/pump", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("status", true)) {
        String status = request->getParam("status", true)->value();
        pumpStatus = (status == "on");
        digitalWrite(RELAY_PUMP_PIN, pumpStatus ? HIGH : LOW);
        if (pumpStatus) {
            playSpeakerNotification("pump_on"); 
        } else {
            playSpeakerNotification("stable_conditions");
        }
        request->send(200, "application/json", "{\"status\":\"success\",\"pumpStatus\":" + String(pumpStatus) + "}");
    } else {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'status' parameter\"}");
    }
});


    server.on("/relay/automatic", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("status", true)) {
            String status = request->getParam("status", true)->value();
            automaticPump = (status == "on");
            request->send(200, "application/json", "{\"status\":\"success\",\"automaticPump\":" + String(automaticPump) + "}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'status' parameter\"}");
        }
    });

    // Curtain endpoints
    server.on("/curtain", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("action", true)) {
        String action = request->getParam("action", true)->value();
        if (action == "open") {
            curtainStatus = true;
            curtainAction(1, 8192); 
            playSpeakerNotification("open_curtain"); 
            request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Curtain is opening\"}");
        } else if (action == "close") {
            curtainAction(-1, 8192); 
            curtainStatus = false;
            playSpeakerNotification("close_curtain"); 
            request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Curtain is closing\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid 'action' value\"}");
        }
    } else {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'action' parameter\"}");
    }
});
 server.on("/stepper/automatic", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("status", true)) {
            String status = request->getParam("status", true)->value();
            automaticCurtain = (status == "on");
            playSpeakerNotification(automaticCurtain ? "open_curtain" : "close_curtain");
            request->send(200, "application/json", "{\"status\":\"success\",\"automaticCurtain\":" + String(automaticCurtain) + "}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'status' parameter\"}");
        }
    });

    // Servo endpoints
    server.on("/servo", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("direction", true)) {
            String direction = request->getParam("direction", true)->value();
            if (direction == "up") {
                servo1Angle = constrain(servo1Angle + 15, 0, 180);
                servo1.write(servo1Angle);
                request->send(200, "application/json", "{\"status\":\"success\",\"servo1Angle\":" + String(servo1Angle) + "}");
            } else if (direction == "down") {
                servo1Angle = constrain(servo1Angle - 15, 0, 180);
                servo1.write(servo1Angle);
                request->send(200, "application/json", "{\"status\":\"success\",\"servo1Angle\":" + String(servo1Angle) + "}");
            } else if (direction == "left") {
                servo2Angle = constrain(servo2Angle - 15, 0, 180);
                servo2.write(servo2Angle);
                request->send(200, "application/json", "{\"status\":\"success\",\"servo2Angle\":" + String(servo2Angle) + "}");
            } else if (direction == "right") {
                servo2Angle = constrain(servo2Angle + 15, 0, 180);
                servo2.write(servo2Angle);
                request->send(200, "application/json", "{\"status\":\"success\",\"servo2Angle\":" + String(servo2Angle) + "}");
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid 'direction' value\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'direction' parameter\"}");
        }
    });
    // Bật/tắt đèn LED
server.on("/led", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("status", true)) {
        String status = request->getParam("status", true)->value();
        if (status == "on") {
            ledStatus = true;
            digitalWrite(LED_PIN, HIGH);
            request->send(200, "application/json", "{\"status\":\"success\",\"ledStatus\":true}");
        } else if (status == "off") {
            ledStatus = false;
            digitalWrite(LED_PIN, LOW);
            request->send(200, "application/json", "{\"status\":\"success\",\"ledStatus\":false}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid 'status' value\"}");
        }
    } else {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'status' parameter\"}");
    }
});

// Tự động bật/tắt đèn LED dựa vào ánh sáng
server.on("/light/automatic", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("status", true)) {
        String status = request->getParam("status", true)->value();
        automaticLight = (status == "on");
        request->send(200, "application/json", "{\"status\":\"success\",\"automaticLight\":" + String(automaticLight) + "}");
    } else {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'status' parameter\"}");
    }
});
server.on("/setThreshold", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("type", true) && request->hasParam("value", true)) {
            String type = request->getParam("type", true)->value();
            int value = request->getParam("value", true)->value().toInt();
            if (type == "soilMoisture") {
                soilMoistureThreshold = value;
            } else if (type == "temperatureHigh") {
                temperatureHighThreshold = value;
            } else if (type == "temperatureLow") {
                temperatureLowThreshold = value;
            } else if (type == "waterLevel") {
                waterLevelThreshold = value;
            } else if (type == "light") {
                lightThreshold = value;
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid threshold type\"}");
                return;
            }
            String response = "{\"status\":\"success\",\"" + type + "Threshold\":" + String(value) + "}";
            request->send(200, "application/json", response);
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'type' or 'value' parameter\"}");
        }
    });
    server.begin();
}
void handleAutomation() {
    // Automatic Pump Control
    if (automaticPump) {
        int soilMoisture = adc1_get_raw(MOISTURE_CHANNEL);
        if (soilMoisture > soilMoistureThreshold && !pumpStatus) {
            digitalWrite(RELAY_PUMP_PIN, HIGH); // Turn the pump ON
            pumpStatus = true;                 // Update pump status
            playSpeakerNotification("low_humidity");
        } else if (soilMoisture <= soilMoistureThreshold && pumpStatus) {
            digitalWrite(RELAY_PUMP_PIN, LOW); // Turn the pump OFF
            pumpStatus = false;                // Update pump status
            playSpeakerNotification("high_humidity");
        }
    }
    if (automaticFan) {
        float temperature = dht.readTemperature();
        if (temperature > temperatureHighThreshold && fanStatus == 0) {
            analogWrite(FAN_PWM_PIN, 255); 
            fanStatus = 255;             
            playSpeakerNotification("high_temperature");
        } else if (temperature <= temperatureLowThreshold && fanStatus > 0) {
            analogWrite(FAN_PWM_PIN, 0); 
            fanStatus = 0;          
            playSpeakerNotification("low_temperature");
        }
    }
    if (automaticCurtain) {
        int waterLevel = adc1_get_raw(WATER_LEVEL_CHANNEL);
        if (waterLevel > waterLevelThreshold && !curtainStatus) {
            curtainAction(1, 8192);
            curtainStatus = false;
            playSpeakerNotification("close_curtain");
        } else if (waterLevel <= waterLevelThreshold && curtainStatus) {
            curtainAction( -1, 8192);
            curtainStatus = true;
            playSpeakerNotification("open_curtain");
        }
    }
    if (automaticLight) {
    int lightLevel = adc1_get_raw(LIGHT_CHANNEL);
    if (lightLevel > lightThreshold) {
        ledStatus = true;
        digitalWrite(LED_PIN, HIGH); 
    } else if (lightLevel <= lightThreshold ) {
        ledStatus = false;
        digitalWrite(LED_PIN, LOW); 
    }
  }

}

int readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return pulseIn(ECHO_PIN, HIGH) * 0.034 / 2;
}
void playSpeakerNotification(String action) {
    int index = 0;
    if (action == "low_humidity") index = 1;
    else if (action == "high_humidity") index = 2;
    else if (action == "high_temperature") index = 3;
    else if (action == "low_temperature") index = 4;
    else if (action == "low_light") index = 5;
    else if (action == "pump_on") index = 6;
    else if (action == "stable_conditions") index = 7;
    else if (action == "open_fan") index = 8;
    else if (action == "off_fan") index = 9;
    else if (action == "open_curtain") index = 10;
    else if (action == "close_curtain") index = 11;
    if (index > 0) {
        myDFPlayer.play(index);  
        Serial.println("Playing audio index: " + String(index));
    } else {
        Serial.println("No valid audio file for action: " + action);
    }
}
void curtainAction(int direction, int steps) {
    curtainStepsRemaining = steps;
    curtainDirection = direction;
    curtainStepper.move(curtainDirection * steps);
}
