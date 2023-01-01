#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include "WeatherTypes.h"

const char URI_OWM[] PROGMEM = "http://api.openweathermap.org/data/2.5/group?id=";
const char URI_OWM_APIKEY[] PROGMEM = "&units=metric&cnt=1&APPID=";
const char URI_OWM_LANG[] PROGMEM = "&lang=en";

const char ERROR_NO_API_KEY[] PROGMEM = "No API key present";
const char ERROR_NO_RESPONSE[] PROGMEM = "API connection failed";
const char ERROR_UNEXPECTED_RESPONSE[] PROGMEM = "Unexpected response code: ";
const char ERROR_EMPTY_RESPONSE[] PROGMEM = "Empty response";
const char ERROR_JSON_DESERIALIZATION[] PROGMEM = "JSON deserialization failed";
const char ERROR_NO_VALID_DATA[] PROGMEM = "No valid data in JSON document ";

class OWMClient
{
    private:
        WiFiClient *_wifiClient;

        String errorMsg = "";
        boolean errorIsPresent = true;

        WeatherStruct weather;

        String getWeatherIcon(int id) const;
        int roundToInt(float value);
        int getWindSpeedInKph(float wind) const;
        void setErrorMsg(String msg);
        void resetErrorMsg();
        String formatWindSpeed(float windSpeed);
        String formatTemperature(float temp);

    public:
        OWMClient(WiFiClient &wifiClient);
        String getErrorMsg();
        boolean getErrorIsPresent();
        int8_t getWeather(unsigned int cityId, String apiKey);
        String getCity();
        String getCountry();
        float getTemperature();
        float getTempFeelslike();
        String getFromatedTemperature();
        unsigned int getHumidity();
        String getFromatedFeelsLike();
        String getFromatedWindSpeed();
        String getDescription();
        String getCondition();
        String getWeatherIconScreen();
        String getWeatherWebIcon();
        float getWind();
};
