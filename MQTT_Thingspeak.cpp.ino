#include <ESP8266WiFi.h>        // * Wifi connection for ESP8266 board
#include "mqtt_secrets.h"       // * Credential for MQTT and Wifi connection
#include <DHT.h>                // * Library for DHT11
#include <ESP8266HTTPClient.h>  // * HTTP Client library for ESP8266 board
#include <WiFiClientSecure.h>   // * Secure client for HTTPS requests

const String apiKey = CHANNEL_WRITE_API_KEY;
const String thingSpeakServer = "http://api.thingspeak.com/update";
const String emailServer = "https://careful-niki-cau-ban-ten-minh-a46ce20a.koyeb.app/api/v1/emails/notify";
const String dataServerUrl = "https://example.com/data"; // Replace with your actual data server URL

#define DHTPIN D2         // * DHT11 Data pin
#define DHTTYPE DHT11     // * Sensor type
#define Relay D0          // * Relay control pin
DHT dht(DHTPIN, DHTTYPE); // * Initialize a DHT object with D2 data pin and DHT sensor type

#define THRESHOLD_TEMP 29
#define RELAY_DURATION_IN_SEC 1

struct Data {
  float temperature;
  float humid;
}

void setup() {
  Serial.begin(115200); // * Initialize Serial with 115200 bit/s
  
  WiFi.begin(SECRET_WIFI_NAME, SECRET_WIFI_PASSWORD); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi ...");
  }

  Serial.println("Connected to WiFi");
  
  pinMode(Relay, OUTPUT); // * Configuring Relay as output of ESP8266 board
  dht.begin();
}

void getDataFromServer() {
  WiFiClientSecure httpsClient;
  httpsClient.setInsecure();
  HTTPClient httpClient;
  
  httpClient.begin(httpsClient, dataServerUrl);
  int httpCode = httpClient.GET();

  if (httpCode > 0) {
    String payload = httpClient.getString();
    Serial.println("Data received from server:");
    Serial.println(payload);
  } else {
    Serial.println("Failed to get data from server. Error: " + String(httpClient.errorToString(httpCode).c_str()));
  }
  
  httpClient.end();
}

void sendDataToThingSpeakServer(float temperature, float humid) {
  WiFiClient wifiClient;
  HTTPClient http;

  const String thingSpeakHttpPostRequest = thingSpeakServer + "?api_key=" + apiKey + "&field1=" + String(temperature) + "&field2=" + String(humid);
  http.begin(wifiClient, thingSpeakHttpPostRequest);

  int thingSpeakHttpCode = http.POST("");
  if (thingSpeakHttpCode > 0) {
    String payload = http.getString();
    Serial.println("ThingSpeak Server request sent successfully.");
    Serial.println("Server response code: " + String(thingSpeakHttpCode));
  } else {
    Serial.println("Failed to send HTTP POST request. Error: " + String(http.errorToString(thingSpeakHttpCode).c_str()));
  }

  http.end();
}

void requestToSendEmail(float temperature, float humid) {
  WiFiClientSecure httpsClient;
  httpsClient.setInsecure();
  HTTPClient httpClientForSendingEmail;

  const String emailSubject = "Humid+and+Temperature+Alert";
  const String emailContent = "Temperature+=+" + String(temperature) + "+|+Humid+=+" + String(humid);
  String mailingHttpsServerGetRequest = String(emailServer) + "?to=" + NOTIFICATION_RECEIVER + "&subject=" + emailSubject + "&content=" + emailContent;

  httpClientForSendingEmail.begin(httpsClient, mailingHttpsServerGetRequest);
  int mailingServiceHttpsCode = httpClientForSendingEmail.GET();
  if (mailingServiceHttpsCode > 0) {
    String getPayload = httpClientForSendingEmail.getString();
    Serial.println("Mailing Server request sent successfully.");
    Serial.println("Server response code: " + String(mailingServiceHttpsCode));
    Serial.println("Response payload: " + getPayload);
  } else {
    Serial.println("Failed to send HTTPS GET request. Error: " + String(httpClientForSendingEmail.errorToString(mailingServiceHttpsCode).c_str()));
  }

  httpClientForSendingEmail.end();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Cannot read data from DHT sensor!");
    return;
  }

  Serial.print("Humid: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C");

  delay(2000);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Trying to reconnect...");
    return;
  }

  if (t >= THRESHOLD_TEMP) {
    Serial.println("Turning on Relay");
    digitalWrite(Relay, LOW);
    delay(RELAY_DURATION_IN_SEC * 1000);
    digitalWrite(Relay, HIGH);
    Serial.println("Turned off Relay");
  } else {
    digitalWrite(Relay, HIGH);
  }

  sendDataToThingSpeakServer(t, h);

  requestToSendEmail(t, h);

  getDataFromServer();

  Serial.println("Waiting 8 seconds to send next request ...");
  delay(8000);
}