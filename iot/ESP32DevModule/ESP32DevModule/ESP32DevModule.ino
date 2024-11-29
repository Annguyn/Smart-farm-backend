#include <Wire.h>
#include <DHT.h>
#include <ESPAsyncWebSrv.h>
#include <WiFi.h>
#include <DFRobotDFPlayerMini.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <driver/adc.h> 

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

DHT dht(DHTPIN, DHTTYPE);
DFRobotDFPlayerMini myDFPlayer;
HardwareSerial mySerial(1);
AsyncWebServer server(80);

const char *ssid = "ASPIRE";
const char *password = "12345678";
String esp8266_ip = "anfarm8266.local";

bool pumpStatus = false, curtainStatus = false, fanStatus = false;
bool automaticPump = false, automaticCurtain = false, automaticFan = false;
int soundCounter = 0;

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
  void sendRequest(String endpoint, String payload = "");

  dht.begin();
  mySerial.begin(9600, SERIAL_8N1, SPEAKER_RX_PIN, SPEAKER_TX_PIN);  // RX = 33, TX = 32
  if (myDFPlayer.begin(mySerial)) {
    Serial.println("DFPlayer Mini initialized successfully.");
    myDFPlayer.volume(30);
  } else {
    Serial.println("DFPlayer Mini initialization failed!");
    Serial.println("1. Kiểm tra kết nối RX/TX.");
    Serial.println("2. Đảm bảo thẻ nhớ microSD đúng định dạng.");
  }

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  if (MDNS.begin("anfarm32")) {
    Serial.println("mDNS responder started at http://anfarm32.local");
  }


  adc1_config_width(ADC_WIDTH_BIT_12); 
  adc1_config_channel_atten(MOISTURE_CHANNEL, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(LIGHT_CHANNEL, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(SOUND_CHANNEL, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(WATER_LEVEL_CHANNEL, ADC_ATTEN_DB_11); 

  setupEndpoints();
}


void loop() {
  handleAutomation();
}

void setupEndpoints() {
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
    response += "\"automaticCurtain\":" + String(automaticCurtain) + "}";
    request->send(200, "application/json", response);
  });
  server.on("/fan/set", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("speed", true)) {
      int speed = request->getParam("speed", true)->value().toInt();
      handleFan(speed, request);
    } else {
      request->send(400, "text/plain", "Missing 'speed' parameter");
    }
  });
  server.on("/fan/automatic/on", HTTP_POST, [](AsyncWebServerRequest *request) {
    automaticFan = true;
    request->send(200, "text/plain", "Fan automatic mode set to ON");
  });

  server.on("/fan/automatic/off", HTTP_POST, [](AsyncWebServerRequest *request) {
    automaticFan = false;
    request->send(200, "text/plain", "Fan automatic mode set to OFF");
  });

  server.on("/pump/on", HTTP_POST, [](AsyncWebServerRequest *request) { handlePump(true, request); });
  server.on("/pump/off", HTTP_POST, [](AsyncWebServerRequest *request) { handlePump(false, request); });
  server.on("/pump/automatic/on", HTTP_POST, [](AsyncWebServerRequest *request) {
    automaticPump = true; 
    request->send(200, "text/plain", "Pump automatic mode set to ON");
  });

  server.on("/pump/automatic/off", HTTP_POST, [](AsyncWebServerRequest *request) {
    automaticPump = false; 
    request->send(200, "text/plain", "Pump automatic mode set to OFF");
  });
  server.on("/curtain/open", HTTP_POST, [](AsyncWebServerRequest *request) { handleCurtain(true, request); });
  server.on("/curtain/close", HTTP_POST, [](AsyncWebServerRequest *request) { handleCurtain(false, request); });
  server.on("/curtain/automatic/on", HTTP_POST, [](AsyncWebServerRequest *request) {
    automaticCurtain = true; 
    request->send(200, "text/plain", "Curtain automatic mode set to ON");
  });
  server.on("/curtain/automatic/off", HTTP_POST, [](AsyncWebServerRequest *request) {
    automaticCurtain = false; 
    request->send(200, "text/plain", "Curtain automatic mode set to OFF");
  });

  server.begin();
}

void handleAutomation() {
    if (automaticPump) {
        int soilMoisture = adc1_get_raw(MOISTURE_CHANNEL);
        if (soilMoisture > 2000 && !pumpStatus) {
            handlePump(true, nullptr);
            playSpeakerNotification("low_humidity");
        } else if (soilMoisture <= 2000 && pumpStatus) {
            handlePump(false, nullptr);
            playSpeakerNotification("high_humidity");
        }
    }

    if (automaticFan) {
        float temperature = dht.readTemperature();
        if (temperature > 35 && !fanStatus) {
            handleFan(255, nullptr);
            playSpeakerNotification("high_temperature");
        } else if (temperature <= 20 && fanStatus) {
            handleFan(0, nullptr);
            playSpeakerNotification("low_temperature");
        }
    }

    if (automaticCurtain) {
        int waterLevel = adc1_get_raw(WATER_LEVEL_CHANNEL);
        if (waterLevel > 2000 && !curtainStatus) {
            handleCurtain(true, nullptr);
        } else if (waterLevel <= 2000 && curtainStatus) {
            handleCurtain(false, nullptr);
        }
    }
}




void handleFan(int speed, AsyncWebServerRequest *request) {
  fanStatus = (speed > 0); 
  speed = constrain(speed, 0, 255); 
  String payload = "speed=" + String(speed);
  Serial.println("Sending fan control request with payload: " + payload);

  sendRequest("/fan/set", payload);

  playSpeakerNotification(fanStatus ? "open_fan" : "off_fan");

  if (request) {
    String response = "Fan speed set to " + String(speed);
    request->send(200, "text/plain", response);
    Serial.println("Response sent to client: " + response);
  }
}

void handlePump(bool status, AsyncWebServerRequest *request) {
    pumpStatus = status;
    sendRequest("/pump/" + String(status ? "on" : "off"), "");
    playSpeakerNotification(status ? "pump_on" : "stable_conditions");  
    if (request) request->send(200, "text/plain", String("Pump is ") + (status ? "ON" : "OFF"));
}

void handleCurtain(bool status, AsyncWebServerRequest *request) {
    curtainStatus = status;
    sendRequest("/curtain/" + String(status ? "open" : "close"), "");
    playSpeakerNotification(status ? "open_curtain" : "close_curtain"); 
    if (request) request->send(200, "text/plain", String("Curtain is ") + (status ? "Opening" : "Closing"));
}



void sendRequest(String endpoint, String payload) {
  HTTPClient http;
  String url = "http://" + esp8266_ip + endpoint;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(payload); 
  if (httpCode > 0) {
    Serial.println("HTTP Response Code: " + String(httpCode));
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.println("HTTP Request Failed: " + String(httpCode));
  }
  http.end();
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




