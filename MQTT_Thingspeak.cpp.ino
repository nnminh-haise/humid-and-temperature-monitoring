#include <ESP8266WiFi.h>        // * Include ESP8266 for wifi connection
#include "mqtt_secrets.h"       // * Include credential information for local wifi connection and MQTT
#include <DHT.h>                // * Include library for DHT sensor for detecting temperature and humidity
#include <ESP8266HTTPClient.h>  // * Include HTTPClient for making HTTP requests

String apikey = CHANNEL_WRITE_API_KEY;                   // * API write Key provided by MQTT service
const char* server = "http://api.thingspeak.com/update"; // * ThingSpeak update URL

#define DHTPIN D2         // * D2 pin of ESP8266 is connected with DHT11 sensor data wire
#define DHTTYPE DHT11     // * Sensor type is DHT11
#define Relay D0          // * D7 pin of ESP8266 is connected with relay control
DHT dht(DHTPIN, DHTTYPE); // * Initialize a DHT object with D2 data pin and DHT sensor type

#define THRESHOLD_TEMP 29
#define RELAY_DURATION_IN_SEC 1

void setup() {
  Serial.begin(115200); // * Initialize Serial port with 115200 bit/s
  
  WiFi.begin(SECRET_WIFI_NAME, SECRET_WIFI_PASSWORD); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi ...");
  }

  Serial.println("Connected to WiFi");
  
  pinMode(Relay, OUTPUT); // * Configuring Relay as output (D7 pin of ESP8266)
  dht.begin();            // * Begin running DHT sensor
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // * Unit C

  if (isnan(h) || isnan(t)) {
    Serial.println("Cannot read data from DHT sensor!");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient wifiClient;
    HTTPClient http;
    
    // * Sample request url: http://api.thingspeak.com/update?api_key=UULXX7OEH50YBQ1Y&field1=12&field2=12
    String url = String(server) + "?api_key=" + apikey + "&field1=" + String(t) + "&field2=" + String(h);
    
    http.begin(wifiClient, url); // * Initialize the HTTP request (use the WiFiClient object)
    
    int httpCode = http.POST(""); // * Send HTTP POST request

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("POST request sent successfully.");
      Serial.println("Server response code: " + String(httpCode));
    } else {
      Serial.println("Failed to send POST request. Error: " + String(http.errorToString(httpCode).c_str()));
    }

    http.end();

    // * Control the relay based on temperature
    if (t >= THRESHOLD_TEMP) {
      Serial.println("Turning on Relay");
      digitalWrite(Relay, LOW);  // * Turn on relay
      delay(RELAY_DURATION_IN_SEC * 1000);
      digitalWrite(Relay, HIGH); // * Turn off relay
    } else {
      digitalWrite(Relay, HIGH); // * Keep relay off if temperature is below threshold
    }
  } else {
    Serial.println("WiFi disconnected. Trying to reconnect...");
  }

  // * Show result on Serial display
  Serial.print("Humid: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C");
  Serial.println("Waiting 10 seconds to read data from sensor and send next request ...");
  
  delay(10000); // * Delay 10 seconds (10,000 milliseconds) for the next request & next sensor read
}
