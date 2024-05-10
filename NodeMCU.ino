#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "ThingSpeak.h"

WiFiClient client_thingSpeak;
const char* ssid = "your_wifi_ssid"; // replace with your WiFi SSID
const char* password = "your_wifi_password"; // replace with your WiFi password
const char* host = "api.ipify.org";

#define SECRET_CH_ID "your id"
#define SECRET_WRITE_APIKEY "your_apikey"

unsigned long myChannelNumber = SECRET_CH_ID;
const char* myWriteAPIKey = SECRET_WRITE_APIKEY;

String getExternalIP() {
  WiFiClient client;
  if (!client.connect(host, 80)) {
    Serial.println("Failed to connect to server");
    return "Failed to connect";
  }
  client.print(String("GET /?format=text HTTP/1.1\r\n") +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  delay(500);

  String response = "";
  while (client.available()) {
    char c = client.read();
    response += c;
  }
  client.stop();

  int indexStart = response.indexOf("\r\n\r\n") + 4;
  int indexEnd = response.indexOf("\n", indexStart);
  String ipAddress = response.substring(indexStart, indexEnd);
  return ipAddress;
}

void setup() {
  Serial.begin(9600);
  delay(10);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client_thingSpeak);
}

void loop() {
  Serial.println("Getting public IP address...");
  String ipAddress = getExternalIP();
  Serial.println("Public IP address: " + ipAddress);

  WiFiClient client;
  if (!client.connect(host, 80)) {
    Serial.println("Connection to ip-api.com failed");
    return;
  }

  client.print(String("GET /json/") + ipAddress + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  while (!client.available()) {
    delay(100);
  }

  String response = "";
  while (client.available()) {
    char c = client.read();
    response += c;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  float latitude = doc["lat"];
  float longitude = doc["lon"];
  Serial.print("Latitude: ");
  Serial.println(latitude, 6);
  Serial.print("Longitude: ");
  Serial.println(longitude, 6);

  ThingSpeak.setField(1, latitude);
  ThingSpeak.setField(2, longitude);
  ThingSpeak.setStatus("Location updated");

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("Channel update successful.");
  } else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

  delay(5000); // Delay for 30 seconds
}
