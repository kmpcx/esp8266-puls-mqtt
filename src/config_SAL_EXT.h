// --------------------- Sensor Settings

const char* MQTT_LOCATION = "berger";
const char* PLANT = "SAL-EXT";

#define MACHINE_PIN  3 //SW02 = 5 //SW01 = 13 //LF = 3

//Reboot Settings
const int rebootHours = 24; //Reboot after X hours
const int thresholdStoppedMinutes = 10; //When for X minutes no pulses detected

// --------------------- Network Settings

// #define WIFI_SSID "SELFINDWL"
// #define WIFI_PASSWORD ""
#define WIFI_SSID "BUS-IOT-EXT"
#define WIFI_PASSWORD ""

bool IPStatic = false;
IPAddress staticIP(10,49,13,103); //ESP static ip
IPAddress gateway(10,49,13,1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(10,49,13,1);  //DNS

#define updateServer "http://update.location/"


// --------------------- MQTT Settings

// #define MQTT_HOST IPAddress((0, 0, 0, 0)
// #define MQTT_PORT 1883
// #define MQTT_SECURE true
// #define MQTT_USERNAME "berger"
// #define MQTT_PASSWORD ""

#define MQTT_HOST IPAddress(0, 0, 0, 0)
#define MQTT_PORT 1883
#define MQTT_SECURE true
#define MQTT_USERNAME "lf-self-mqtt-sensor"
#define MQTT_PASSWORD ""


// --------------------- Function Settings

bool usePing = true;
bool useNTP = false; //Not in use
bool useOTAonStart = false;
bool checkWiFi = false; //Not in use

bool resetWiFi = false; //Reset Sensor when connecting to old "oldSSID"
const char* oldSSID = "";

bool testing = false; //Send every second a pulse