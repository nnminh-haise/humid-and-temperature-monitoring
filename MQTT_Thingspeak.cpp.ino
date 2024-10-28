#include <ESP8266WiFi.h>        // * Wifi connection for ESP8266 board
#include "mqtt_secrets.h"       // * Credential for MQTT and Wifi connection
#include <DHT.h>                // * Library for DHT11
#include <ESP8266HTTPClient.h>  // * HTTP Client library for ESP8266 board

const String apiKey = CHANNEL_WRITE_API_KEY;             // * API write Key provided by MQTT service
const char* server = "http://api.thingspeak.com/update"; // * ThingSpeak update URL
const char* emailServer = "https://careful-niki-cau-ban-ten-minh-a46ce20a.koyeb.app/api/v1/emails/notify";

#define DHTPIN D2         // * DHT11 Data pin
#define DHTTYPE DHT11     // * Sensor type
#define Relay D0          // * Relay control pin
DHT dht(DHTPIN, DHTTYPE); // * Initialize a DHT object with D2 data pin and DHT sensor type

#define THRESHOLD_TEMP 29
#define RELAY_DURATION_IN_SEC 1

void setup() {
  Serial.begin(115200); // * Initialize Serial with 115200 bit/s
  
  WiFi.begin(SECRET_WIFI_NAME, SECRET_WIFI_PASSWORD); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi ...");
  }

  Serial.println("Connected to WiFi");
  
  pinMode(Relay, OUTPUT); // * Configuring Relay as output of ESP8266 board
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
    
    // * Sample POST request url
    String url = String(server) + "?api_key=" + apiKey + "&field1=" + String(t) + "&field2=" + String(h);
    
    http.begin(wifiClient, url); // * Initialize the HTTP request
    int httpCode = http.POST(""); // * Send HTTP POST request

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("POST request sent successfully.");
      Serial.println("Server response code: " + String(httpCode));
    } else {
      Serial.println("Failed to send POST request. Error: " + String(http.errorToString(httpCode).c_str()));
    }

    http.end();

    // * Sample GET request with query params
    HTTPClient getHttp;

    getHttp.begin(wifiClient, getUrl);
    String getUrl = String(emailServer) + "?to=" + String(NOTIFICATION_RECEIVER) + "&subject=Humid+and+Temperature+Alert&content=Temperature+=+" + String(t) + "+-+Humid+=+" + String(h);
    Serial.println("url:", getUrl);
    int getHttpCode = getHttp.GET();

    if (getHttpCode > 0) {
      String getPayload = getHttp.getString();
      Serial.println("GET request sent successfully.");
      Serial.println("Server response code: " + String(getHttpCode));
      Serial.println("Response payload: " + getPayload);
    } else {
      Serial.println("Failed to send GET request. Error: " + String(getHttp.errorToString(getHttpCode).c_str()));
    }

    getHttp.end();

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