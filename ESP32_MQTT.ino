// This example uses an ESP32 Development Board
// to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: https://shiftr.io/try.
//
// by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt

#include <WiFiClientSecure.h>
#include <MQTT.h>
#include "ConnectionConfiguration.h"

WiFiClientSecure net;
MQTTClient client;

unsigned long lastMillis = 0;
unsigned long numMessages = 0;

void connectWifi() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("/hello");
  // client.unsubscribe("/hello");
}

void scanWifi() {
  int n = WiFi.scanNetworks();
  if (n == 0) {
      Serial.println("no networks found");
  } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
          // Print SSID and RSSI for each network found
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(WiFi.SSID(i));
          Serial.print(" (");
          Serial.print(WiFi.RSSI(i));
          Serial.print(")");
          Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
          delay(10);
      }
  }
  Serial.println("");  
}

void beginWifi(const char ssid[], const char password[]) {
  if(strlen(password) > 0) {
    Serial.println("Connecting with PSK");
    WiFi.begin(ssid, password);
  } else {
    Serial.println("Connecting without PSK");
    WiFi.begin(ssid);    
  }
}

void messageReceived(String &topic, String &payload) {
  numMessages++;
  Serial.print("incoming: ");
  Serial.print(numMessages);
  Serial.println(" " + topic + " - " + payload);
}

void setup() {
  Serial.begin(115200);
  Serial.print("This devices MAC address is ");
  Serial.println(WiFi.macAddress());
  scanWifi();
  beginWifi(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported by Arduino.
  // You need to set the IP address directly.
  //
  // MQTT brokers usually use port 8883 for secure connections.
  client.begin("broker.shiftr.io", 8883, net);
  client.onMessage(messageReceived);

  connectWifi();
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connectWifi();
  }

  // publish a message roughly every second.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    client.publish("/hello", "world");
  }
}
