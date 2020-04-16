#include <ESP8266WiFi.h>
#include <Bounce2.h>
#include <Ticker.h>
#include <PubSubClient.h>
#include <ESP8266Ping.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

#include configFile

// --------------------- Configuration ---------------------

char NODE_ID[16];
const char* VERSION = "S1.1";

#define LEDRED 0
#define LEDGREEN 16

long lastPulse = 0;

//Bounce definition
Bounce debouncer = Bounce();
int oldInput = -1;

//Variables
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
Ticker testTimer;

Ticker rebootTimer;
int wifiDisconnects = 0;
int mqttDisconnects = 0;

String startDate = "0";

char startString[60];

int count = 0;
int maxCount = 1000000;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
long lastReconnectAttempt = 0;

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

void mqttTopics()
{
  strcpy(MQTT_TOPIC, MQTT_LOCATION);
  strcat(MQTT_TOPIC, "/");
  strcat(MQTT_TOPIC, NODE_ID);

  strcpy(MQTT_TOPIC_REQ, MQTT_TOPIC);
  strcat(MQTT_TOPIC_REQ, "/2");

  strcpy(MQTT_TOPIC_PULSE, MQTT_TOPIC);
  strcat(MQTT_TOPIC_PULSE, "/1/2");

  strcpy(MQTT_TOPIC_COUNT, MQTT_TOPIC);
  strcat(MQTT_TOPIC_COUNT, "/1/3");

  strcpy(MQTT_TOPIC_HEARTBEAT, MQTT_TOPIC);
  strcat(MQTT_TOPIC_HEARTBEAT, "/3/0");

  strcpy(MQTT_TOPIC_ONLINE, MQTT_TOPIC);
  strcat(MQTT_TOPIC_ONLINE, "/3/1");

  strcpy(MQTT_TOPIC_ONLINE_WILL, MQTT_TOPIC_ONLINE);
  strcat(MQTT_TOPIC_ONLINE_WILL, "/online");

  strcpy(MQTT_SUBSCRIBE, MQTT_LOCATION);
  strcat(MQTT_SUBSCRIBE, "/sub/");
  strcat(MQTT_SUBSCRIBE, NODE_ID);
  // strcat(MQTT_SUBSCRIBE,"/#");

  strcpy(MQTT_SUBSCRIBE_PLANT, MQTT_LOCATION);
  strcat(MQTT_SUBSCRIBE_PLANT, "/sub/");
  strcat(MQTT_SUBSCRIBE_PLANT, PLANT);
  // strcat(MQTT_SUBSCRIBE_PLANT,"/#");
}

void pulse(bool test)
{
  //get current state of machine
  debouncer.update();
  // Get the update value
  int input = debouncer.read();
  if (test)
  {
    input = 0;
  }
  if (input != oldInput)
  {
    oldInput = input;
    if (input == 0)
    {
      digitalWrite(LEDRED, HIGH);
      if (count >= maxCount)
      {
        count = 0;
      }
      count++;
      lastPulse = millis();
      if (mqttClient.connected())
      {
        String req = "pulse";
        uint16_t pubPulse = mqttClient.publish(MQTT_TOPIC_PULSE, (char *)req.c_str(), false);
        Serial.print("Publishing pulse, packetId: ");
        Serial.println(pubPulse);

        String tmp = String(count);
        uint16_t pubCount = mqttClient.publish(MQTT_TOPIC_COUNT, (char *)tmp.c_str(), true);
        Serial.print("Publishing count, packetId: ");
        Serial.println(pubCount);
      }
      else
      {
        Serial.print("Offline produced part - count: ");
        Serial.println(count);
      }
    }
    else
    {
      digitalWrite(LEDRED, LOW);
    }
  }
}

void testPulse(){
   pulse(true);
}

void sendHeartbeat()
{
  long rssi = WiFi.RSSI();
  String tmp = String(rssi);
  uint16_t pubHeartbeat = mqttClient.publish(MQTT_TOPIC_HEARTBEAT, (char *)tmp.c_str(), false);
  Serial.print("Publishing heartbeat, packetId: ");
  Serial.println(pubHeartbeat);
}

void ota()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    ESPhttpUpdate.setLedPin(LEDGREEN, LOW);
    char link[25 + strlen(updateServer)];

    strcpy(link, updateServer);
    strcat(link, "/");
    if (updateOTAPlant)
    {
      updateOTAPlant = false;
      strcat(link, PLANT);
    }
    else
    {
      strcat(link, NODE_ID);
    }
    strcat(link, ".bin");
    Serial.println(link);

    t_httpUpdate_return response = ESPhttpUpdate.update(client, link);
    //t_httpUpdate_return response = ESPhttpUpdate.update(client, updateServer, 80, link, VERSION);
    //t_httpUpdate_return response = ESPhttpUpdate.update(client, updateServer, 80, link);
    String req;
    switch (response)
    {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      req = "OTA failed";
      mqttClient.publish(MQTT_TOPIC_REQ, (char *)req.c_str(), false);
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      req = "OTA no updates";
      mqttClient.publish(MQTT_TOPIC_REQ, (char *)req.c_str(), false);
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      req = "OTA done";
      mqttClient.publish(MQTT_TOPIC_REQ, (char *)req.c_str(), false);
      break;
    }
  }
}

void rebootTime()
{
  rebootTimer.detach();
  unsigned long now = millis();
  if (now - lastPulse > (thresholdStoppedMinutes * 60000) || lastPulse == 0)
  {
    Serial.print("Reboot now");
    ESP.restart();
  }
  else
  {
    rebootTimer.attach(60 * 5, rebootTime);
  }
}

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");

  if (IPStatic)
  {
    WiFi.config(staticIP, subnet, gateway, dns);
  }
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    wifiDisconnects++;
    // Serial.print("wifiDisconnects: ");
    // Serial.println(wifiDisconnects);
    if(wifiDisconnects > 60){
      ESP.restart();
    }
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void onMessage(char *topic, byte *payload, unsigned int len)
{
  char msg[len + 1];
  strlcpy(msg, (char *)payload, len + 1);
  if (strcmp(msg, (char *)"ping") == 0)
  {
    String req = "Pong";
    Serial.println("REQ: ping");
    mqttClient.publish(MQTT_TOPIC_REQ, (char *)req.c_str(), false);
  }
  else if (strcmp(msg, (char *)"version") == 0)
  {
    Serial.println("REQ: version");
    mqttClient.publish(MQTT_TOPIC_REQ, VERSION, false);
  }
  else if (strcmp(msg, (char *)"plant") == 0)
  {
    Serial.println("REQ: plant");
    mqttClient.publish(MQTT_TOPIC_REQ, PLANT, false);
  }
  else if (strcmp(msg, (char *)"reboot") == 0)
  {
    Serial.println("REQ: reboot");
    String req = "Reboot Sensor";
    mqttClient.publish(MQTT_TOPIC_REQ, (char *)req.c_str(), false);
    ESP.restart();
  }
  else if (strcmp(msg, (char *)"update") == 0)
  {
    Serial.println("REQ: update");
    String req = "Start OTA Update";
    mqttClient.publish(MQTT_TOPIC_REQ, (char *)req.c_str(), false);
    updateOTA = true;
  }
  else if (strcmp(msg, (char *)"update_plant") == 0)
  {
    Serial.println("REQ: update");
    String req = "Start OTA Update Plant";
    mqttClient.publish(MQTT_TOPIC_REQ, (char *)req.c_str(), false);
    updateOTAPlant = true;
    updateOTA = true;
  }
}

boolean reconnectMQTT()
{
  String clientId = NODE_ID;
  Serial.print("Attempting MQTT connection...");
  mqttDisconnects++;
  // Serial.print("mqttDisconnects: ");
  // Serial.println(mqttDisconnects);
  if(mqttDisconnects > 10){
    ESP.restart();
  }
  if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD, MQTT_TOPIC_ONLINE_WILL, 2, true, "0"))
  {
    
    Serial.println("connected");
    mqttClient.publish(MQTT_TOPIC_ONLINE_WILL, "1", true);
    if (firstConnect)
    {
      firstConnect = false;
      strcpy(startString, VERSION);
      strcat(startString, "-");
      strcat(startString, PLANT);

      mqttClient.publish(MQTT_TOPIC_ONLINE, startString, true);

      char topic[strlen(MQTT_TOPIC_ONLINE) + 9];
      strcpy(topic, MQTT_TOPIC_ONLINE);
      strcat(topic, "/plant");
      mqttClient.publish(topic, PLANT, false);
      strcpy(topic, MQTT_TOPIC_ONLINE);
      strcat(topic, "/version");
      mqttClient.publish(topic, VERSION, false);
    }
    mqttClient.subscribe(MQTT_SUBSCRIBE);
    mqttClient.subscribe(MQTT_SUBSCRIBE_PLANT);
    Serial.print("Subscribed to ");
    Serial.println(MQTT_SUBSCRIBE);
    Serial.print("Subscribed to ");
    Serial.println(MQTT_SUBSCRIBE_PLANT);
  }
  return mqttClient.connected();
}

void setup()
{
  Serial.begin(9600);
  //Serial.setDebugOutput(true);

  digitalWrite(LEDGREEN, HIGH);
  digitalWrite(LEDRED, HIGH);
  for (uint8_t t = 6; t > 0; t--)
  {
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
  mac.replace(":", "");
  Serial.println(mac);
  mac.toCharArray(NODE_ID, 16);

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

  rebootTimer.attach(60 * 60 * rebootHours, rebootTime);

  connectToWifi();

  lastReconnectAttempt = 0;
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMessage);

  delay(1500);

  heartbeatTimer.attach(10, sendHeartbeat);
  if(testing){
    testTimer.attach(1, testPulse);
  } 
}

void statusLED()
{
  if (mqttClient.connected())
  {
    digitalWrite(LEDGREEN, HIGH);
  }
  else
  {
    unsigned long now = millis();
    unsigned int timeLED = 1750;
    if (WiFi.status() == WL_CONNECTED)
    {
      if (usePing && (now - lastOnlineCheck) >= 15000)
      {
        lastOnlineCheck = now;
        IPAddress ip(8, 8, 8, 8);
        sensorOnline = Ping.ping(ip, 1);
      }
      if (!usePing)
      {
        timeLED = 666;
      }
      else if (sensorOnline)
      {
        timeLED = 150;
      }
      else
      {
        timeLED = 1000;
        if (LEDFast)
        {
          timeLED = 200;
        }
        if (countStatusLED >= 5)
        {
          countStatusLED = 0;
          LEDFast = !LEDFast;
        }
      }
    }
    else
    {
      timeLED = 1750;
    }
    if ((now - lastLED) > timeLED)
    {
      countStatusLED++;
      lastLED = now;
      if (ledGreenState == LOW)
      {
        ledGreenState = HIGH;
      }
      else
      {
        ledGreenState = LOW;
      }
      digitalWrite(LEDGREEN, ledGreenState);
    }
  }
}

void loop()
{
  if (!mqttClient.connected())
  {
    long now = millis();
    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnectMQTT())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    mqttClient.loop();
  }

  pulse(false);
  statusLED();
  if (updateOTA)
  {
    updateOTA = false;
    ota();
  }
}
