#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h> 

#define DHTPIN D1              
#define DHTTYPE DHT11          
#define RELAY_PIN D2           

const char* ssid = "DOAN VAN ANH";          
const char* password = "0979377346";  

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80); 

String pumpStatus = "off";

void setup() {
    Serial.begin(115200);
    dht.begin();
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); 

    WiFi.begin(ssid, password);
    Serial.print("Đang kết nối đến Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Kết nối thành công!");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP()); 

    server.on("/status", HTTP_GET, handleStatus);
    server.on("/control", HTTP_POST, handleControl);
    server.on("/sensor", HTTP_GET, handleSensorData);

    server.begin(); 
}

void loop() {
    server.handleClient(); 

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Không đọc được DHT11!");
        return;
    }

    Serial.print("Nhiệt độ: ");
    Serial.print(temperature);
    Serial.print(" °C, Độ ẩm: ");
    Serial.print(humidity);
    Serial.println(" %");

    delay(2000); 
}

void handleStatus() {
    server.send(200, "application/json", "{\"pump_status\":\"" + pumpStatus + "\"}");
}

void handleControl() {
    Serial.println("Handling control request...");
    if (server.hasArg("action")) {
        String action = server.arg("action");
        Serial.println("Received action: " + action); 
        if (action == "on") {
            digitalWrite(RELAY_PIN, HIGH);
            pumpStatus = "on";
        } else if (action == "off") {
            digitalWrite(RELAY_PIN, LOW);
            pumpStatus = "off";
        }
    } else {
        Serial.println("No action specified"); 
    }
    server.send(200, "application/json", "{\"pump_status\":\"" + pumpStatus + "\"}");
}



void handleSensorData() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
  
    String json = "{\"temperature\":" + String(temperature) +
                  ", \"humidity\":" + String(humidity) + 
                  ", \"pump_status\":\"" + pumpStatus + "\"}";

    server.send(200, "application/json", json);
}
