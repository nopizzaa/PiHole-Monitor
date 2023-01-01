/** The MIT License (MIT)
Copyright (c) 2018 David Payne
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <ESP8266WiFi.h>
#include <EasyButton.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <LittleFS.h> 
#include <FS.h>
#include <ArduinoOTA.h>

#include "WeatherStationFonts.h"
#include "DFRobot_SHT20.h"
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "CustomIcons.h"
#include "OWMClient.h"
#include "Sun.h"
#include "PiHoleClient.h"
#include "Config.h"

#define SPIFFS LittleFS
#define SCREEN_WIDHT 128

#define BTN_LONG_PRESSED 2000
#define BUTTON_PIN 2
#define I2C_DISPLAY_ADDRESS 0x3c
#define SDA_PIN 4
#define SCL_PIN 5
#define WEBSERVER_PORT 80
#define NOTIFICATION_LED_PIN LED_BUILTIN
#define VERSION "a3.0"
#define HOSTNAME "PIMON-%X"
#define NTP_UPDATE_INTERVAL 7200000L
#define WEATHER_UPDATE_INTERVAL 900000L
#define SENSOR_MQTT_UPDATE_INTERVAL 30000L
#define UPDATE_PIHOLESUM_MQTT_RECONET_INTERVAL 60000L
#define UPDATE_PIHOLE_GRAPH_INTERVAL 600000L
#define SERIAL_BAUD_RATE 115200
#define TARGET_FPS 60

#define FIRMWARE_PREFIX "esp8266-piholemonitor-temperature-sensor"
#define AVAILABILITY_ONLINE "online"
#define AVAILABILITY_OFFLINE "offline"
#define MQTT_PORT 1883

char identifier[24];
char MQTT_TOPIC_AVAILABILITY[128];
char MQTT_TOPIC_STATE[128];
char MQTT_TOPIC_COMMAND[128];

char MQTT_TOPIC_AUTOCONF_WIFI_SENSOR[128];
char MQTT_TOPIC_AUTOCONF_SHT20_TEMPERATURE_SENSOR[128];
char MQTT_TOPIC_AUTOCONF_SHT20_HUMIDITY_SENSOR[128];

// Global variables
boolean autoTransition = true;
boolean nightTimeIsActive = false;
boolean weatherErrorWasPresent = false;
boolean piholeErrorWasNotPresent = false;
boolean mqttServerEnabled = true;

unsigned long lastWeatherUpdate = 0;
unsigned long lastSensorMqttUpdate = 0;
unsigned long lastPiholeUpdateMqttReconet = 0;
unsigned long lastPiholeGraphUpdate = 0;

// Sensors
float temperature = 0.0f;
float humidity = 0.0f;