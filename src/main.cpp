#include <Arduino.h>
#include <ESP8266WiFi.h>
// #include <ArduinoOTA.h>
#include <UniversalTelegramBot.h> //https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

#include "credentials.hpp"


#define DEBUG false
#define Serial if(DEBUG)Serial

// #define ATOMIC_FS_UPDATE



// Set GPIOs for OUT_RELAY and PIR Motion Sensor
#define PIR_SENSOR 0
#define OUT_RELAY  2

long unsigned int out_relay_delay_s = 30;

// Telegram variables
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_telegram_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_telegram_client);
// Checks for new messages every 1 second.
int botRequestDelay = 5000;
unsigned long lastTimeBotRan;




// Timer: Auxiliary variables
unsigned long current_time = millis();
unsigned long last_trigger = 0;
boolean start_timer = false;


boolean alarm_mode = true;
boolean is_active  = true;
boolean motion_detected = false;


void serial_init();
void wifi_init();
// void ota_init();
void gpio_init();
void telegram_client_init();

void IRAM_ATTR detects_movement();
void check_movement();
void bot_alarm();
void bot_get_update();
void bot_message_handler(int numNewMessages);


void setup() {
  serial_init();
  wifi_init();
  telegram_client_init();
  // ota_init();
  gpio_init();
}


void loop() {
  // ArduinoOTA.handle();
  check_movement();
  bot_alarm();
  bot_get_update();
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
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}


// void ota_init() {
//   // Port defaults to 8266
//   // ArduinoOTA.setPort(8266);

//   // Hostname defaults to esp8266-[ChipID]
//   // ArduinoOTA.setHostname("myesp8266");

//   // No authentication by default
//   // ArduinoOTA.setPassword("admin");

//   // Password can be set with it's md5 value as well
//   // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
//   // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

//   ArduinoOTA.onStart([]() {
//     String type;
//     if (ArduinoOTA.getCommand() == U_FLASH) {
//       type = "sketch";
//     } else { // U_FS
//       type = "filesystem";
//     }

//     // NOTE: if updating FS this would be the place to unmount FS using FS.end()
//     Serial.println("Start updating " + type);
//   });
//   ArduinoOTA.onEnd([]() {
//     Serial.println("\nEnd");
//   });
//   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//     Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
//   });
//   ArduinoOTA.onError([](ota_error_t error) {
//     Serial.printf("Error[%u]: ", error);
//     if (error == OTA_AUTH_ERROR) {
//       Serial.println("Auth Failed");
//     } else if (error == OTA_BEGIN_ERROR) {
//       Serial.println("Begin Failed");
//     } else if (error == OTA_CONNECT_ERROR) {
//       Serial.println("Connect Failed");
//     } else if (error == OTA_RECEIVE_ERROR) {
//       Serial.println("Receive Failed");
//     } else if (error == OTA_END_ERROR) {
//       Serial.println("End Failed");
//     }
//   });

//   ArduinoOTA.begin();
//   Serial.printf("OTA Ready, free space available: %.f bytes\n",(float)ESP.getFreeSketchSpace());
// }


void gpio_init() {
  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(PIR_SENSOR, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(PIR_SENSOR), detects_movement, RISING);

  // Set OUT_RELAY to LOW
  pinMode(OUT_RELAY, OUTPUT);
  digitalWrite(OUT_RELAY, LOW);
}


void telegram_client_init() {
  configTime(0, 0, "pool.ntp.org");  // get UTC time via NTP
  secured_telegram_client.setTrustAnchors(&cert);     // Add root certificate for api.telegram.org
  Serial.println("Telegram client is ready.");
  bot.sendMessage(CHAT_ID, "Wardrobe bot started up", "");

  const String commands = F("["
    "{\"command\":\"stat\", \"description\":\"Device current status\"},"
    "{\"command\":\"alarm\", \"description\":\"Send motion detection message\"},"
    "{\"command\":\"up\", \"description\":\"Increase 10 sec light delay\"},"
    "{\"command\":\"down\", \"description\":\"Decrease 10 sec light delay\"},"
    "{\"command\":\"on\", \"description\":\"Light on\"},"
    "{\"command\":\"off\",\"description\":\"Light off\"}" // no comma on last command
    "]");
  bot.setMyCommands(commands);
}


void IRAM_ATTR detects_movement() {
  // Checks if motion was detected, sets OUT_RELAY HIGH and starts a timer
  Serial.println("MOTION DETECTED!!!");
  motion_detected = true;
  if (is_active) {
    digitalWrite(OUT_RELAY, HIGH);
  }
  start_timer = true;
  last_trigger = millis();
}


void check_movement() {
  // Turn off the OUT_RELAY after the number of seconds defined in the timeSeconds variable
  if(start_timer && (millis() - last_trigger > (out_relay_delay_s * 1000))) {
    Serial.println("Motion stopped...");
    digitalWrite(OUT_RELAY, LOW);
    start_timer = false;
  }
}


void bot_alarm() {
  if (alarm_mode && motion_detected) {
    bot.sendMessage(CHAT_ID, "MOTION DETECTED!", "");
    motion_detected = false;
  }
}


// Handle what happens when you receive new messages
void bot_message_handler(int numNewMessages) {
  Serial.println("bot_message_handler");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user.", "");
      continue;
    }

    // Print the received message
    String text = bot.messages[i].text;
    Serial.print(bot.messages[i].from_name);
    Serial.print(": ");
    Serial.println(text);

    if (text == "/stat") {
      String alarm_stat = alarm_mode ? "on" : "off";
      String light_stat = is_active ? "on" : "off";
      String msg = (
        "alarm: " + alarm_stat + "\n" +
        "light: " + light_stat + "\n" +
        "delay: " + String(out_relay_delay_s) + " sec"
      );
      bot.sendMessage(chat_id, msg, "");
    }

    if (text == "/up") {
      out_relay_delay_s += 10;
      bot.sendMessage(chat_id, "set delay to " + String(out_relay_delay_s) + " sec.", "");
    }

    if (text == "/down") {
      out_relay_delay_s -= 10;
      bot.sendMessage(chat_id, "set delay to " + String(out_relay_delay_s) + " sec.", "");
    }

    if (text == "/alarm") {
      if (alarm_mode) {
        alarm_mode = false;
        bot.sendMessage(chat_id, "alarm OFF", "");
      } else {
        alarm_mode = true;
        bot.sendMessage(chat_id, "alarm ON", "");
      }
    }

    if (text == "/off") {
        is_active = false;
        bot.sendMessage(chat_id, "Wardrobe light OFF", "");
        digitalWrite(OUT_RELAY, LOW);
    }

    if (text == "/on") {
        is_active = true;
        bot.sendMessage(chat_id, "Wardrobe light ON", "");
    }

  }
}

void bot_get_update() {
    if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("Got response.");
      bot_message_handler(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
