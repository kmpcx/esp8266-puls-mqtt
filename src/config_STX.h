// --------------------- Sensor Settings

const char* MQTT_LOCATION = "ber";
const char* PLANT = "STX";
const char* SKETCH_VERSION = "17";

#define MACHINE_PIN  3 //SW02 = 5 //SW01 = 13 //LF = 3

//Reboot Settings
const int rebootHours = 24; //Reboot after X hours
const int thresholdStoppedMinutes = 10; //When for X minutes no pulses detected

// --------------------- Network Settings

#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "***REMOVED***"

bool IPStatic = false;
IPAddress staticIP(10,49,13,103); //ESP static ip
IPAddress gateway(10,49,13,1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(10,49,13,1);  //DNS

#define updateServer "http://update.location/"


// --------------------- MQTT Settings

#define MQTT_HOST IPAddress(0,0,0,0)
#define MQTT_PORT 1883
#define MQTT_SECURE true
#define MQTT_USERNAME "ber"
#define MQTT_PASSWORD "***REMOVED***"


// --------------------- Function Settings

bool usePing = true;
bool useNTP = false; //Not in use
bool useOTAonStart = false;
bool checkWiFi = false;