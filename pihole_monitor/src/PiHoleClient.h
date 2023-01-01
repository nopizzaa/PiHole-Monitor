#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include "PiHoleTypes.h"

const char URI_SCHEME[] PROGMEM = "http://";
const char URI_PIHOLE_API_SUMMARY[] PROGMEM = "/admin/api.php?summary&auth=";
const char URI_PIHOLE_API_TOP_BLOCKED[] PROGMEM = "/admin/api.php?topClientsBlocked=3&auth=";
const char URI_PIHOLE_API_OVER_TIME_DATA[] PROGMEM = "/admin/api.php?overTimeData10mins&auth=";

const char ERROR_NO_PIHOLE_HOST[] PROGMEM = "Pihole URL/Host is empty";
const char ERROR_INVALID_PIHOLE_PORT[] PROGMEM = "Pihole api port number is incorect";
const char ERROR_NO_PIHOLE_API_TOKEN[] PROGMEM = "API token missing";
const char ERROR_NO_PIHOLE_API_RESPONSE[] PROGMEM = "Failed to connect to api";
const char ERROR_EMPTY_PIHOLE_API_RESPONSE[] PROGMEM = "Empty response";
const char ERROR_PIHOLE_JSON_DESERIALIZATION[] PROGMEM = "JSON deserialization failed";
const char ERROR_PIHOLE_TOO_SMALL_RESPONSE[] PROGMEM = "Content not met the expected size";

class PiHoleClient 
{
    private:
        WiFiClient *_wifiClient;

        String SummaryErrorMessage;

        PiHoleQuerryError TopCLientError;
        PiHoleQuerryError GraphError;

        ClientBlocked blockedClients[3];

        int blocked[144] = {0};

        uint8_t graphScreenWidht = 0;
        uint8_t blockedCount = 0;
        int stopIndex = 0;
        int blockedHigh = 0;

        PiholeDataStruct piHoleData;

        PiHoleStatus piHoleStatus;

        void setSummaryError(String message);
        void resetSummaryError();
        void setTopClienError(String message);
        void resetTopClienError();
        void setGraphError(String message);
        void resetGraphError();
        void resetClientsBlocked();
        void resetBlockedGraphData();
        void setStatusEnum(String status);
        String getPiholeUri(String host, int port);
        
    public:
        PiHoleClient(WiFiClient &wifiClient, uint8_t screenWidht);
        int8_t getPiHoleSummary(String host, int port, String apiToken);
        int8_t getTopClientsBlocked(String host, int port, String apiToken);
        int8_t getGraphData(String host, int port, String apiToken);
        String getDomainsBeingBlocked();
        String getAdsBlockedToday();
        String getAdsPercentageToday();
        String getClientsEverSeen();
        String getUniqueClients();
        String getPiHoleStatus();
        int *getBlockedAds();
        String getTopClientBlocked(int i);
        int getTopClientBlockedCount(int i);
        int getBlockedHigh();
        int getStopIndex();
        uint8_t getBlockedCount();
        uint8_t getStatusEnum();
        String getSummaryErrorMsg();
        String getTopCLientErrorMsg();
        String getGraphErrorMsg();
};
