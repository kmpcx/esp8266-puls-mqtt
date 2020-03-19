#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <Bounce2.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266Ping.h>

#include configFile

// --------------------- Configuration --------------------- Don't change anything below this line ---------------------

char NODE_ID[16];

#define LEDRED 0
#define LEDGREEN 16

long lastPulse = 0;

//Bounce definition
Bounce debouncer = Bounce();  
int oldInput = -1;

//Variables
bool wifiConnected = false;
bool mqttConnected = false;
int mqttDisconnects = 0;
bool updateOTA = false;
bool updateOTAPlant = false;
bool firstConnect = true;

unsigned long lastLED = 0;
int countStatusLED = 0;
bool LEDFast = true;
bool sensorOnline = true;
unsigned long lastOnlineCheck = 0;
unsigned long lastWifiConnect = 0;
int ledGreenState = LOW; 

unsigned long lastHeartbeat = 0;
Ticker heartbeatTimer;

Ticker rebootTimer;

String startDate = "0";

char startString[60];

int count = 0;
int maxCount = 1000000;

// --------------------- MQTT Topics Settings

char MQTT_TOPIC[20];

char MQTT_SUBSCRIBE[25];
char MQTT_SUBSCRIBE_PLANT[25];
char MQTT_TOPIC_REQ[25];

char MQTT_TOPIC_HEARTBEAT[25];
char MQTT_TOPIC_PULSE[25];
char MQTT_TOPIC_COUNT[25];
char MQTT_TOPIC_ONLINE[25];
char MQTT_TOPIC_ONLINE_WILL[35];

void mqttTopics(){
  strcpy(MQTT_TOPIC,MQTT_LOCATION);
  strcat(MQTT_TOPIC,"/");
  strcat(MQTT_TOPIC,NODE_ID);

  strcpy(MQTT_TOPIC_REQ,MQTT_TOPIC);
  strcat(MQTT_TOPIC_REQ,"/2");
  
  strcpy(MQTT_TOPIC_PULSE,MQTT_TOPIC);
  strcat(MQTT_TOPIC_PULSE,"/1/2");

  strcpy(MQTT_TOPIC_COUNT,MQTT_TOPIC);
  strcat(MQTT_TOPIC_COUNT,"/1/3");
  
  strcpy(MQTT_TOPIC_HEARTBEAT,MQTT_TOPIC);
  strcat(MQTT_TOPIC_HEARTBEAT,"/3/0");

  strcpy(MQTT_TOPIC_ONLINE,MQTT_TOPIC);
  strcat(MQTT_TOPIC_ONLINE,"/3/1");

  strcpy(MQTT_TOPIC_ONLINE_WILL, MQTT_TOPIC_ONLINE);
  strcat(MQTT_TOPIC_ONLINE_WILL, "/online");

  strcpy(MQTT_SUBSCRIBE,MQTT_LOCATION);
  strcat(MQTT_SUBSCRIBE,"/sub/");
  strcat(MQTT_SUBSCRIBE,NODE_ID);
  strcat(MQTT_SUBSCRIBE,"/#");

  strcpy(MQTT_SUBSCRIBE_PLANT,MQTT_LOCATION);
  strcat(MQTT_SUBSCRIBE_PLANT,"/sub/");
  strcat(MQTT_SUBSCRIBE_PLANT,PLANT);
  strcat(MQTT_SUBSCRIBE_PLANT,"/#");
}

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

ESP8266WiFiMulti WiFiMulti;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void sendHeartbeat(){
  long rssi = WiFi.RSSI();
  String tmp = String(rssi);
  uint16_t pubHeartbeat = mqttClient.publish(MQTT_TOPIC_HEARTBEAT, 1, false, (char*) tmp.c_str());
  Serial.print("Publishing heartbeat with QoS 1, packetId: ");
  Serial.println(pubHeartbeat);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");

  if(IPStatic){
    WiFi.config(staticIP, subnet, gateway, dns);
  }
  WiFi.mode(WIFI_STA);
  
  if (WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    wifiConnected = true;
    connectToMqtt();
  }
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  wifiConnected = true;
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  wifiConnected = false;
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  mqttConnected = true;
  mqttDisconnects = 0;
  mqttClient.subscribe(MQTT_SUBSCRIBE, 2);
  mqttClient.subscribe(MQTT_SUBSCRIBE_PLANT, 2);
  Serial.print("Subscribed to ");
  Serial.println(MQTT_SUBSCRIBE);
  Serial.print("Subscribed to ");
  Serial.println(MQTT_SUBSCRIBE_PLANT);
  
  char topic[strlen(MQTT_TOPIC_ONLINE) + 9];
  strcpy(topic, MQTT_TOPIC_ONLINE);
  strcat(topic, "/plant");
  mqttClient.publish(topic, 2, false, PLANT);
  strcpy(topic, MQTT_TOPIC_ONLINE);
  strcat(topic, "/version");
  mqttClient.publish(topic, 2, false, SKETCH_VERSION);

  heartbeatTimer.attach(10, sendHeartbeat);

  strcpy(startString,SKETCH_VERSION);
  strcat(startString,"-");
  strcat(startString,PLANT);
  // mqttClient.publish(MQTT_TOPIC_ONLINE, 2, true, startString);
  mqttClient.publish(MQTT_TOPIC_ONLINE_WILL, 2, true, "1");
  if(firstConnect){
    firstConnect = false;
    mqttClient.publish(MQTT_TOPIC_ONLINE, 2, true, startString);
  }


  if(useOTAonStart){
    updateOTA = true;
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  mqttDisconnects++;
  if(mqttDisconnects > 500){
    ESP.restart();
  }
  if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
    Serial.println("TCP_DISCONNECTED");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION) {
    Serial.println("MQTT_UNACCEPTABLE_PROTOCOL_VERSION");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED) {
    Serial.println("MQTT_IDENTIFIER_REJECTED");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE) {
    Serial.println("MQTT_SERVER_UNAVAILABLE");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS) {
    Serial.println("MQTT_MALFORMED_CREDENTIALS");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED) {
    Serial.println("MQTT_NOT_AUTHORIZED");
  } else if (reason == AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE) {
    Serial.println("ESP8266_NOT_ENOUGH_SPACE");
  } else if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
    Serial.println("TLS_BAD_FINGERPRINT");
  } else {
    Serial.println("MQTT Connection problem");
  }
  
  mqttConnected = false;
  heartbeatTimer.detach();

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  char msg[len+1];
  strlcpy(msg, payload, len+1);
  if(strcmp(msg, (char*) "ping") == 0){
    String req = "Pong";
    Serial.println("REQ: ping");
    mqttClient.publish(MQTT_TOPIC_REQ, 1, false, (char*) req.c_str());
  } else if(strcmp(msg, (char*) "version") == 0){
    Serial.println("REQ: version");
    mqttClient.publish(MQTT_TOPIC_REQ, 1, false, SKETCH_VERSION);
  } else if(strcmp(msg, (char*) "plant") == 0){
    Serial.println("REQ: version");
    mqttClient.publish(MQTT_TOPIC_REQ, 1, false, PLANT);
  } else if(strcmp(msg, (char*) "reboot") == 0){
    Serial.println("REQ: reboot");
    String req = "Reboot Sensor";
    mqttClient.publish(MQTT_TOPIC_REQ, 1, false, (char*) req.c_str());
    ESP.restart();
  } else if(strcmp(msg, (char*) "update") == 0){
    Serial.println("REQ: update");
    String req = "Start OTA Update";
    mqttClient.publish(MQTT_TOPIC_REQ, 1, false, (char*) req.c_str());
    updateOTA = true;
  } else if(strcmp(msg, (char*) "update_plant") == 0){
    Serial.println("REQ: update");
    String req = "Start OTA Update Plant";
    mqttClient.publish(MQTT_TOPIC_REQ, 1, false, (char*) req.c_str());
    updateOTAPlant = true;
    updateOTA = true;
  }
}

void onMqttPublish(uint16_t packetId) {
  
}

void ota(){
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    WiFiClient client;
    ESPhttpUpdate.setLedPin(LEDGREEN, LOW);
    char link[25 + strlen(updateServer)];
    
    strcpy(link, updateServer);
    strcat(link, "/");
    if(updateOTAPlant){
      updateOTAPlant = false;
      strcat(link, PLANT);
    } else {
      strcat(link, NODE_ID);
    }
    strcat(link, ".bin");
    Serial.println(link);
    
    t_httpUpdate_return response = ESPhttpUpdate.update(client, link);
    //t_httpUpdate_return response = ESPhttpUpdate.update(client, updateServer, 80, link, SKETCH_VERSION);
    //t_httpUpdate_return response = ESPhttpUpdate.update(client, updateServer, 80, link);
    String req;
    switch (response) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        req = "OTA failed";
        mqttClient.publish(MQTT_TOPIC_REQ, 1, false, (char*) req.c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        req = "OTA no updates";
        mqttClient.publish(MQTT_TOPIC_REQ, 1, false, (char*) req.c_str());
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        req = "OTA done";
        mqttClient.publish(MQTT_TOPIC_REQ, 1, false, (char*) req.c_str());
        break;
      }
    }
}

void rebootTime(){
  rebootTimer.detach();
  unsigned long now = millis();
  if(now - lastPulse > (thresholdStoppedMinutes*60000) || lastPulse == 0){
    Serial.print("Reboot now");
    ESP.restart();
  } else {
    rebootTimer.attach(60*5, rebootTime);
  }
}

void setup() {
  Serial.begin(9600);
  //Serial.setDebugOutput(true);

  digitalWrite(LEDGREEN, HIGH);
  digitalWrite(LEDRED, HIGH);
  for (uint8_t t = 6; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(250);
  }
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED, LOW);
  
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  String mac = WiFi.macAddress();
  mac.replace(":","");
  Serial.println(mac);
  mac.toCharArray(NODE_ID,16);

  mqttTopics();

  // Setup the machine
  pinMode(MACHINE_PIN, INPUT);
  // Activate internal pull-up
  digitalWrite(MACHINE_PIN, HIGH);

  // After setting up the input, setup debouncer
  debouncer.attach(MACHINE_PIN);
  debouncer.interval(200);

  pinMode(LEDRED, OUTPUT);
  pinMode(LEDGREEN, OUTPUT);

  rebootTimer.attach(60*60*rebootHours, rebootTime);
  
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USERNAME, MQTT_PASSWORD);
  mqttClient.setKeepAlive(5);
  mqttClient.setClientId(NODE_ID);
  mqttClient.setWill(MQTT_TOPIC_ONLINE_WILL, 2, true, "0", 0);

  WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  
  connectToWifi();
}

void statusLED() {
  if(mqttConnected){
    digitalWrite(LEDGREEN, HIGH);
  } else {
    unsigned long now = millis();
    unsigned int timeLED = 1750;
    if(wifiConnected){
      if(usePing && (now - lastOnlineCheck) >= 15000){
        lastOnlineCheck = now;
        IPAddress ip (8, 8, 8, 8);
        sensorOnline = Ping.ping(ip, 1);
      }
      if(!usePing){
        timeLED = 666;
      } else if(sensorOnline){
        timeLED = 150;
      } else {
        timeLED = 1000;
        if(LEDFast){
          timeLED = 200;
        }
        if(countStatusLED >= 5){
          countStatusLED = 0;
          LEDFast = !LEDFast;
        }      
      }
    } else {
      timeLED = 1750;
    }
    if ((now - lastLED) > timeLED) {
      countStatusLED++;
      lastLED = now;
      if (ledGreenState == LOW) {
        ledGreenState = HIGH;
      } else {
        ledGreenState = LOW;
      }
      digitalWrite(LEDGREEN, ledGreenState);
    }
  }
}

void pulse() {
  //get current state of machine
  debouncer.update();
  // Get the update value
  int input = debouncer.read();

  if (input != oldInput) {
    oldInput = input;
    if ( input == 0) {
      digitalWrite(LEDRED, HIGH);
      if (count >= maxCount){
        count = 0;
      }
      count ++;
      lastPulse = millis();
      if (mqttConnected) {
        String req = "pulse";
        uint16_t pubPulse = mqttClient.publish(MQTT_TOPIC_PULSE, 2, false, (char*) req.c_str());
        Serial.print("Publishing pulse with QoS 2, packetId: ");
        Serial.println(pubPulse);
  
        String tmp = String(count);
        uint16_t pubCount = mqttClient.publish(MQTT_TOPIC_COUNT, 1, true, (char*) tmp.c_str());
        Serial.print("Publishing count with QoS 1, packetId: ");
        Serial.println(pubCount);
      } else {
        Serial.print("Offline produced part - count: ");
        Serial.println(count);
      }
    } else {
      digitalWrite(LEDRED, LOW);
    }
  }
}

void loop() {
  if (!wifiConnected) {
    unsigned long now = millis();
    if (now - lastWifiConnect > 2000) {
        lastWifiConnect = now;
        if(WiFiMulti.run() != WL_CONNECTED){
          Serial.println("WiFi not connected!");
        }
    }
  }
  pulse();
  statusLED();
  if(updateOTA){
    updateOTA = false;
    ota();
  }
}
