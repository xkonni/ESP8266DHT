#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <DHT.h>

extern "C" {
#include "user_interface.h"
}

#define PIN       4
#define DHTTYPE   DHT22
#define DELAY     10000

const char* ssid = "********";
const char* password = "********";
const char* host = "esp8266DHT";

os_timer_t myTimer;
boolean tickOccured;
void timerCallback(void *pArg) {
  tickOccured = true;
}

DHT dht(PIN, DHTTYPE);
WiFiClient wclient;
// Update these with values suitable for your network.
IPAddress MQTTserver(192, 168, 11, 22);
//PubSubClient client(wclient);
PubSubClient client(MQTTserver, 1883, wclient);

void callback (char* topic, byte* payload, uint8_t length) {
  uint16_t i;

  // handle message arrived
  char* myPayload = (char *) malloc(sizeof(char) * (length + 1));
  strncpy(myPayload, (char *) payload, length);
  myPayload[length] = '\0';
  Serial.print(topic);
  // Serial.print(" (");
  // Serial.print(length);
  // Serial.print(") => ");
  Serial.print(" => ");
  Serial.println(myPayload);

  float t = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.println(t);


  // if (! strcmp(topic, "NeoPixel")) {
  //   if (! strncmp((char*) payload, "on", length)) {
  //     // reset brightness and saturation, keep hue
  //     brightness = 255;
  //     saturation = 255;
  //
  //     color = getRGB(hue, saturation, brightness);
  //     for(i=0; i<strip.numPixels(); i++) {
  //       strip.setPixelColor(i, color);
  //     }
  //     strip.show();
  //   }
  //   else {
  //     // turn off, set brightness to 0
  //     brightness = 0;
  //
  //     color = getRGB(hue, saturation, brightness);
  //     for (i=0; i<strip.numPixels(); i++) {
  //       strip.setPixelColor(i, color);
  //     }
  //     strip.show();
  //   }
  // }
  //
  // else if (! strcmp(topic, "NeoPixelBrightness")) {
  //   // [0; 100] -> [0; 255]
  //   brightness = atoi(myPayload) * 255 / 100;
  //   color = getRGB(hue, saturation, brightness);
  //   for (i = 0; i < strip.numPixels(); i++) {
  //     strip.setPixelColor(i, color);
  //   }
  //   strip.show();
  // }
  // else if (! strcmp(topic, "NeoPixelHue")) {
  //   // [0; 360]
  //   hue = atoi(myPayload);
  //   color = getRGB(hue, saturation, brightness);
  //   for(i=0; i<strip.numPixels(); i++) {
  //     strip.setPixelColor(i, color);
  //   }
  //   strip.show();
  // }
  //
  // else if (! strcmp(topic, "NeoPixelSaturation")) {
  //   // [0; 100] -> [0; 255]
  //   saturation = atoi(myPayload) * 255 / 100;
  //   color = getRGB(hue, saturation, brightness);
  //   for (i = 0; i < strip.numPixels(); i++) {
  //     strip.setPixelColor(i, color);
  //   }
  //   strip.show();
  // }
}

void setup() {
  Serial.begin(76800);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(host);

  ArduinoOTA.onStart([]() {
      Serial.println("Start");
      });

  ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
      });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      });

  ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // MQTT callback
  client.setCallback(callback);

  // Timer
  tickOccured = false;
  /*
   * os_timer_setfn - Define a function to be called when the timer fires
   * void os_timer_setfn(
   *   os_timer_t *pTimer,
   *   os_timer_func_t *pFunction,
   *   void *pArg)
   * The pArg parameter is the value registered with the callback function.
   */
  os_timer_setfn(&myTimer, timerCallback, NULL);

  /*
   * os_timer_arm -  Enable a millisecond granularity timer.
   * void os_timer_arm(
   *   os_timer_t *pTimer,
   *   uint32_t milliseconds,
   *   bool repeat)
   */
  os_timer_arm(&myTimer, DELAY, true);

  dht.begin();
}

void loop() {
  ArduinoOTA.handle();
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      if (client.connect("ESP8266: DHT")) {
        client.publish("outTopic", "hello world");
        client.subscribe("DHT");
      }
    }

    if (client.connected()) {
      if (tickOccured) {
        float t = dht.readTemperature();
        // make sure we're receiving a valid number
        if (! isnan(t)) {
          // Serial.print("Temperature: ");
          // Serial.println(t);
          client.publish("DHTTemperature", String(t).c_str());
        }
        tickOccured = false;
      }
      client.loop();
    }
  }
}
