#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "credentials.hpp"


#define DEBUG true
#define Serial if(DEBUG)Serial


void wifi_init();


void setup() {
  Serial.begin(115200);
  wifi_init();
}


void loop() {
  // put your main code here, to run repeatedly:
}


void wifi_init() {
  Serial.print("Connecting: ");
  Serial.println(WIFI_SSID);

  WiFi.hostname("Wardrobe");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
