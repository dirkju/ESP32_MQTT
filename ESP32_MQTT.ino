// This example uses an ESP32 Development Board
// to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: https://shiftr.io/try.

// Libraries
#include <WiFiClientSecure.h>
#include <PubSubClient.h> // PubSubClient
#include <PubSubClientTools.h> // PubSubClientTools dersimn
#include <Thread.h> // ArduinoThread Ivan Seidel
#include <ThreadController.h>

#include "ConnectionConfiguration.h"

#define MQTT_SERVER "broker.shiftr.io"

WiFiClientSecure net;
PubSubClient client(MQTT_SERVER, 1883, net); // see https://docs.shiftr.io/interfaces/mqtt/
PubSubClientTools mqtt(client);

ThreadController threadControl = ThreadController();
Thread thread = Thread();

unsigned long lastMillis = 0;
unsigned long numMessages = 0;
int value = 0;
const String s = "";

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

void connectWifi(const char ssid[], const char password[]) {
  if(strlen(password) > 0) {
    Serial.println("Wifi: Using PSK");
    WiFi.begin(ssid, password);
  } else {
    Serial.println("Wifi: Not using PSK (open Wifi)");
    WiFi.begin(ssid);    
  }

  Serial.print("Wifi: polling for connection");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nWifi: connected!");
}

/** void messageReceived(String &topic, String &payload) {
  numMessages++;
  Serial.print("incoming: ");
  Serial.print(numMessages);
  Serial.println(" " + topic + " - " + payload);
} **/

void publisher() {
  ++value;
  mqtt.publish("test_out/hello_world", s+"Hello World! - No. "+value);
}

void topic1_subscriber(String topic, String message) {
  Serial.println(s+"Message arrived in function 1 ["+topic+"] "+message);
}

void topic2_subscriber(String topic, String message) {
  Serial.println(s+"Message arrived in function 2 ["+topic+"] "+message);
}

void topic3_subscriber(String topic, String message) {
  Serial.println(s+"Message arrived in function 3 ["+topic+"] "+message);
}

void setup() {
  Serial.begin(115200);
  Serial.print("This devices MAC address is ");
  Serial.println(WiFi.macAddress());
  Serial.println("Wifi: Scanning networks");
  scanWifi();
  connectWifi(ssid, pass);

  // Connect to MQTT
  Serial.println("MQTT: "+s+"Connecting to: "+MQTT_SERVER+" ... ");
  if (client.connect("ESP32arduino", "try", "try")) {
//  if (client.connect("ESP32Client01", mqtt_user, mqtt_pass)) {
    Serial.println("MQTT: connected");
    mqtt.subscribe("test_in/foo/bar",  topic1_subscriber);
    mqtt.subscribe("test_in/+/bar",    topic2_subscriber);
    mqtt.subscribe("test_in/#",        topic3_subscriber);
  } else {
    Serial.println("MQTT: " + s + "failed, Client.state = "+client.state());
    delay(5000);
    // error out
    return;
  }

  // Enable Thread
  thread.onRun(publisher);
  thread.setInterval(2000);
  threadControl.add(&thread);
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    Serial.println("MQTT: not connected.");
    delay(10000);
    return;
  }

  threadControl.run();
}
