ESP8266 Sketch for potential free contact sensor
======================

This sketch is meant to let you detecd pulses on a potential free contact and send them via an asyncronous connection to a MQTT broker.
It can be used to monitor every kind of binary state. It can be used to detect machine cycles (counting parts), states of windows to observe if they are closed properly or any relay you can possibly think of.

<img src="https://darwin.kmpc.de/wp-content/uploads/2020/05/IMG_20200529_154809-scaled.jpg" alt="drawing" width="400"/>

## Table of content
- [Setup](#setup)
    - [WiFi](#wifi)
    - [MQTT](#mqtt)
    - [OTA](#ota)
- [PCB](#pcb)


## Setup

All the setup is done in the config file. You can create own config files for different locations/environment.


### WiFi

```
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "Password"

bool IPStatic = false; // true if it is a static IP
IPAddress staticIP(10,49,13,103); //ESP static ip
IPAddress gateway(10,49,13,1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(10,49,13,1);  //DNS
```

### MQTT

You need to define your MQTT broker.
The MQTT Topics are defined in the main.ccp file.

```
#define MQTT_HOST IPAddress(0, 0, 0, 0)
#define MQTT_PORT 1883
#define MQTT_SECURE true
#define MQTT_USERNAME "USER"
#define MQTT_PASSWORD "PASSWORD"
```

### OTA

You need to define a location where the micro controller should ask for a update binary file.

```
#define updateServer "http://location.loc/"
```

## PCB

We designed a PCB board which fits perfectly the need for a potential free contact sensor.
The source for it is located in the PCB folder.

<img src="https://darwin.kmpc.de/wp-content/uploads/2020/05/IMG_20200529_154724-scaled.jpg" alt="drawing" width="400"/>