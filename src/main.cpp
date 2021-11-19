#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include "credentials.hpp"


#define DEBUG true
#define Serial if(DEBUG)Serial

#define ATOMIC_FS_UPDATE

// Set GPIOs for OUT_RELAY and PIR Motion Sensor
#define PIR_SENSOR 0
#define OUT_RELAY  2

#define DELAY_SECONDS 10

// Timer: Auxiliary variables
unsigned long current_time = millis();
unsigned long last_trigger = 0;
boolean start_timer = false;


void serial_init();
void wifi_init();
void ota_init();
void gpio_init();

void IRAM_ATTR detects_movement();
void check_movement();


void setup() {
  serial_init();
  wifi_init();
  ota_init();
  gpio_init();
}


void loop() {
  ArduinoOTA.handle();
  check_movement();
}


void serial_init() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
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
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
}


void ota_init() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
  Serial.printf("OTA Ready, free space available: %.f bytes\n",(float)ESP.getFreeSketchSpace());
}


void gpio_init() {
  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(PIR_SENSOR, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(PIR_SENSOR), detects_movement, RISING);

  // Set OUT_RELAY to LOW
  pinMode(OUT_RELAY, OUTPUT);
  digitalWrite(OUT_RELAY, LOW);
}


void IRAM_ATTR detects_movement() {
  // Checks if motion was detected, sets OUT_RELAY HIGH and starts a timer
  Serial.println("MOTION DETECTED!!!");
  digitalWrite(OUT_RELAY, HIGH);
  start_timer = true;
  last_trigger = millis();
}


void check_movement() {
  current_time = millis();
  // Turn off the OUT_RELAY after the number of seconds defined in the timeSeconds variable
  if(start_timer && (current_time - last_trigger > (DELAY_SECONDS * 1000))) {
    Serial.println("Motion stopped...");
    digitalWrite(OUT_RELAY, LOW);
    start_timer = false;
  }
}
