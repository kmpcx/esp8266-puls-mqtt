ESP8266 Sketch for potential free contact sensor
======================

This sketch is made to dected pulses on a potential free contact and send them to a MQTT broker.
It can be used to monitor every kind of binary state. It can be machine to count parts, it can be a window to observe if it opens or it can be a relay from every controller.

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

### OTA

## PCB