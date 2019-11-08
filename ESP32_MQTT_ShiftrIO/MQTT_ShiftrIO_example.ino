// inspired from Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt
// You can check on your device after a successful
// connection here: https://shiftr.io/try.

#include <WiFi.h>
#include <MQTT.h>
#include <Wire.h>
#include <BH1750FVI.h>

// ESP 32 on-board thermometer
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

String device_id = "ESP-02";
const char ssid[] = "MSFTGUEST";

// ESP32 missing this definition, hence here:
#define LED_BUILTIN 2

// GY-30 light sensor
BH1750FVI lightSensor(BH1750FVI::k_DevModeContLowRes);

WiFiClient net;
MQTTClient mqttClient;

unsigned long lastMillis = 0, loopId = 0;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("wifi connected.\nconnecting MQTT...");
  while (!mqttClient.connect("arduino", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\n MQTT connected!");

  mqttClient.subscribe("/hello");
  mqttClient.subscribe("/mdcc/arduino/*/*");
  mqttClient.subscribe("/mdcc/arduino/command/led");
  // mqttClient.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  // command handler
  if (topic == "/mdcc/arduino/command/led") {
    if (payload == "on") {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    if (payload == "off") {
      digitalWrite(LED_BUILTIN, LOW);
    }
    mqttClient.publish("/mdcc/arduino/command/led/" + device_id, "completed");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid);
  mqttClient.begin("broker.shiftr.io", net);
  mqttClient.onMessage(messageReceived);
  connect();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  lightSensor.begin();
}

void loop() {
  char buf[40];
  
  mqttClient.loop();
  delay(10);  // <- fixes some issues with WiFi stability
  if (!mqttClient.connected()) {
    connect();
  }

  // publish a message roughly every second.
  if (millis() - lastMillis > 2000) {
    lastMillis = millis();
    if((loopId % 2) == 0) {
      mqttClient.publish("/mdcc/arduino/hello", "Hello MDCC Arduino 102!");
      mqttClient.publish("/mdcc/arduino/" + device_id + "/heartbeat", "alive");
    } else {
      float temp = (temprature_sens_read() - 32) / 1.6;
      snprintf(buf, sizeof(buf) - 1, "%f", temp);
      mqttClient.publish("/mdcc/arduino/" + device_id + "/module_temp", buf);
  
      uint16_t lux = lightSensor.GetLightIntensity();
      snprintf(buf, sizeof(buf) - 1, "%d", lux);
      mqttClient.publish("/mdcc/arduino/" + device_id + "/light_lux", buf);
    }
    loopId++;
  }
}
