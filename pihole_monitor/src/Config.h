#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h> 
#include <FS.h>

#define CONFIG_FILE "/config.json"

namespace Config
{    
    int ntpTimeOffset = 0;
    char ntpPoolServerName[80] = "";

    char mqttServer[80] = "";
    char mqttUsername[24] = "";
    char mqttPassword[24] = "";

    String piholeUri = "";
    String piholeApiToken = "";
    unsigned int piholePort = 80;

    unsigned int weatherCityId = 0;
    String weatherApiKey = "";

    boolean nightTimeEnabled = true;
    boolean nightTimeOnAuto = true;

    uint8_t displayBrightness = 255;
    uint8_t nightTimeBrightness = 40;

    unsigned int nightTimeOn = 0;
    unsigned int nightTimeOff = 0;

    float lat = 0.0f;
    float lon = 0.0f;

    int8_t loadConfig() 
    {
        if (!LittleFS.exists(CONFIG_FILE))
        {
            Serial.println("Configuration file does not exist");
            return 1;
        }
    
        File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (!configFile)
        {
            Serial.println("Failed to open configurationfile for reading");
            return 2;
        }

        const size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument config(1024);
        DeserializationError error = deserializeJson(config, buf.get());
        if (error)
        {
            Serial.println("JSON deserialization failed");
            return 2;
        }

        strcpy(ntpPoolServerName,   config["ntp_pool_server"]);
        strcpy(mqttServer,          config["mqtt_server"]);
        strcpy(mqttUsername,        config["mqtt_username"]);
        strcpy(mqttPassword,        config["mqtt_password"]);

        lat                 = config["lat"].as<float>();
        lon                 = config["lon"].as<float>();
        ntpTimeOffset       = config["utc_offset"].as<int>();
        piholeUri           = config["pihole_uri"].as<const char*>();
        piholeApiToken      = config["pihole_api_token"].as<const char*>();
        piholePort          = config["pihole_port"].as<unsigned int>();
        weatherCityId       = config["weather_city_id"].as<unsigned int>();
        weatherApiKey       = config["weather_api_key"].as<const char*>();
        nightTimeEnabled    = config["night_time_enabled"].as<boolean>();
        nightTimeOnAuto     = config["night_time_auto_on"].as<boolean>();
        displayBrightness   = config["brightness"].as<uint8_t>();
        nightTimeBrightness = config["night_brightness"].as<uint8_t>();
        nightTimeOn         = config["night_time_on_at"].as<unsigned int>();
        nightTimeOff        = config["night_time_off_at"].as<unsigned int>();

        return 0;
    }
}