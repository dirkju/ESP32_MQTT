// This example uses an ESP32 Development Board
// to connect to Azure IOT Hub
//

// Libraries
#include <WiFiClientSecure.h>
#include <ESP.h>
#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"
#include "ConnectionConfiguration.h"

// ESP 32 on-board thermometer
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

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

  LOG_VERBOSE("- [%s] event was received. Payload => %s",
              callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

  if (strcmp(callbackInfo->eventName, "Command") == 0) {
    LOG_VERBOSE("- Command name was => %s", callbackInfo->tag);
  }
}

int send_property(IOTContext ctx, const char * key, const char * value) {
  char msg[64] = {0};
  int pos = 0;
  pos = snprintf(msg, sizeof(msg) - 1, "{\"%s\":%s}", key, value);
  return iotc_send_property(ctx, msg, pos);
}

int send_telemetry(IOTContext ctx, const char * key, const double value) {
  char msg[64] = {0};
  int pos = 0;
  pos = snprintf(msg, sizeof(msg) - 1, "{\"%s\":%f}", key, value);
  return iotc_send_telemetry(ctx, msg, pos);
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
      char buf[40];
      int errorCode = 0;
      byte mac[6];
      char macAddr[20];

      lastTick = ms;

      // send onboard telemetry
      errorCode = send_telemetry(context, "magneticfield", hallRead());
      errorCode = send_telemetry(context, "onboardtemp", (temprature_sens_read() - 32) / 1.6);

      if (loopId++ % 10 == 0) {
        // send device properties
        WiFi.macAddress(mac);
        sprintf(macAddr, "\"%2X:%2X:%2X:%2X:%2X:%2X\"", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        errorCode = send_property(context, "mac", macAddr);
        errorCode = send_property(context, "chip_revision", itoa(ESP.getChipRevision(), buf, 10));
        errorCode = send_property(context, "cpu_frq_mhz", itoa(ESP.getCpuFreqMHz(), buf, 10));
        errorCode = send_property(context, "flashchip_size_kb", itoa(ESP.getFlashChipSize() / 1024.0, buf, 10));
      }

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
