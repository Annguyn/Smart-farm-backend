#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define DHTPIN D1               
#define DHTTYPE DHT11           
#define SOIL_MOISTURE_PIN A0    
#define RELAY_PIN D7            
#define ULTRASONIC_TRIGGER_PIN D2 
#define ULTRASONIC_ECHO_PIN D3    

const char* ssid = "Ameriux"; 
const char* password = "hlewluv123";


DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

String pumpStatus = "off"; 
String mode = "manual"; 
const int soilMoistureThreshold = 400; 

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); 

    pinMode(ULTRASONIC_TRIGGER_PIN, OUTPUT); 
    pinMode(ULTRASONIC_ECHO_PIN, INPUT);     

    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/status", HTTP_GET, handleStatus);
    server.on("/control", HTTP_POST, handleControl);
    server.on("/sensor", HTTP_GET, handleSensorData);
    server.on("/mode", HTTP_POST, handleModeChange); 
    server.begin();
}

void loop() {
    server.handleClient();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
        // Serial.println("Failed to read from DHT11");
        return;
    }

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.print("Distance: ");
    Serial.println(readUltrasonicDistance());

    int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
    Serial.print("Soil Moisture: ");
    Serial.println(soilMoisture);

    if (mode == "automatic") {
        if (soilMoisture < soilMoistureThreshold && pumpStatus == "off") {
            Serial.println("Activating pump due to low soil moisture");
            pumpStatus = "on";
            digitalWrite(RELAY_PIN, HIGH); 
        } else if (soilMoisture >= soilMoistureThreshold && pumpStatus == "on") {
            Serial.println("Deactivating pump due to sufficient soil moisture");
            pumpStatus = "off";
            digitalWrite(RELAY_PIN, LOW); 
        }
    }

    delay(2000);
}

void handleStatus() {
    server.send(200, "application/json", "{\"pump_status\":\"" + pumpStatus + "\", \"mode\":\"" + mode + "\"}");
}

void handleControl() {
    if (server.hasArg("action")) {
        String action = server.arg("action");
        if (action == "on") {
            Serial.println("PUMP_ON");
            pumpStatus = "on";
            digitalWrite(RELAY_PIN, HIGH); 
        } else if (action == "off") {
            Serial.println("PUMP_OFF");
            pumpStatus = "off";
            digitalWrite(RELAY_PIN, LOW); 
        }
    }
    server.send(200, "application/json", "{\"pump_status\":\"" + pumpStatus + "\"}");
}

void handleSensorData() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
    long distance = readUltrasonicDistance(); 

    String json = "{\"temperature\":" + String(temperature) +
                  ", \"humidity\":" + String(humidity) + 
                  ", \"soil_moisture\":" + String(soilMoisture) +
                  ", \"distance\":" + String(distance) +
                  ", \"pump_status\":\"" + pumpStatus + "\"}";

    server.send(200, "application/json", json);
}

void handleModeChange() {
    if (server.hasArg("mode")) {
        mode = server.arg("mode");
        Serial.print("Mode changed to: ");
        Serial.println(mode);
    }
    server.send(200, "application/json", "{\"mode\":\"" + mode + "\"}");
}

long readUltrasonicDistance() {
    digitalWrite(ULTRASONIC_TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIGGER_PIN, LOW);

    long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);
    long distance = duration * 0.034 / 2; 
    return distance;
}