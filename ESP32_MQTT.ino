// This example uses an ESP32 Development Board
// to connect to Azure IOT Hub
//

// Libraries
#include <WiFiClientSecure.h>
#include <PubSubClient.h> // PubSubClient http://knolleary.net
#include <PubSubClientTools.h> // PubSubClientTools dersimn
#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"

#include "ConnectionConfiguration.h"

void scan_wifi() {
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

void connect_wifi(const char ssid[], const char password[]) {
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

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
#include "src/connection.h"

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  // ConnectionStatus
  if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
    LOG_VERBOSE("Is connected ? %s (%d)",
                callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO",
                callbackInfo->statusCode);
    isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    return;
  }

  // payload buffer doesn't have a null ending.
  // add null ending in another buffer before print
  AzureIOT::StringBuffer buffer;
  if (callbackInfo->payloadLength > 0) {
    buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
  }

  LOG_VERBOSE("- [%s] event was received. Payload => %s\n",
              callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

  if (strcmp(callbackInfo->eventName, "Command") == 0) {
    LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.print("This devices MAC address is ");
  Serial.println(WiFi.macAddress());
  Serial.println("Wifi: Scanning networks");
  scan_wifi();
  connect_wifi(ssid, pass);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
  Serial.println("IOTC: Connecting client finished");

  if (context != NULL) {
    lastTick = 0; // set timer in the past to enable first telemetry asap.
  }
}

void loop() {
  if (isConnected) {
    unsigned long ms = millis();
    if (ms - lastTick > 10000) {  // send telemetry every 10 seconds
      char msg[64] = {0};
      int pos = 0, errorCode = 0;

      lastTick = ms;
      if (loopId++ % 2 == 0) {  // send telemetry
        pos = snprintf(msg, sizeof(msg) - 1, "{\"accelerometerX\": %d}",
                       10 + (rand() % 20));
        errorCode = iotc_send_telemetry(context, msg, pos);
      } else {  // send property
        pos = snprintf(msg, sizeof(msg) - 1, "{\"dieNumber\":%d}",
                       1 + (rand() % 5));
        errorCode = iotc_send_property(context, msg, pos);
      }
      msg[pos] = 0;

      if (errorCode != 0) {
        LOG_ERROR("Sending message has failed with error code %d", errorCode);
      }
    }

    iotc_do_work(context);  // do background work for iotc
  } else {
    iotc_free_context(context);
    context = NULL;
    connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
  }
}
