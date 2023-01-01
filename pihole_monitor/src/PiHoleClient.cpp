#include "PiHoleClient.h"

PiHoleClient::PiHoleClient(WiFiClient &wifiClient, uint8_t screenWidht)
{
    graphScreenWidht = screenWidht;
    _wifiClient = &wifiClient;
    resetBlockedGraphData();
}

int8_t PiHoleClient::getPiHoleSummary(String piholeUri, int port, String apiToken)
{
    if (piholeUri.length() <= 0)
    {
        setSummaryError(String(ERROR_NO_PIHOLE_HOST));
        return 1;
    }

    if (port <= 0)
    {
        setSummaryError(String(ERROR_INVALID_PIHOLE_PORT));
        return 2;
    }

    if (apiToken.length() <= 0)
    {
        setTopClienError(String(ERROR_NO_PIHOLE_API_TOKEN));
        return 3;
    }

    HTTPClient httpClient;
    const String uri = getPiholeUri(piholeUri, port) + String(URI_PIHOLE_API_SUMMARY) + apiToken;;

    httpClient.begin(*_wifiClient, uri);
    const int responseCode = httpClient.GET();

    if (responseCode <= 0)
    {
        setSummaryError(String(ERROR_NO_PIHOLE_API_RESPONSE));
        return 4;
    }
    else if (responseCode != 200)
    {
        const String msg = String(ERROR_NO_PIHOLE_API_RESPONSE) + String(responseCode);
        setSummaryError(msg);
        return 5;
    }
    else if (httpClient.getSize() <= 0)
    {
        setSummaryError(String(ERROR_EMPTY_PIHOLE_API_RESPONSE));
        return 6;
    }

    const String response = httpClient.getString();
    httpClient.end();

    const size_t bufferSize = 1230;
    DynamicJsonDocument doc(bufferSize);

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        setSummaryError(String(ERROR_PIHOLE_JSON_DESERIALIZATION));
        return 7;
    }

    if (measureJson(doc) <= bufferSize / 2)
    {
        setSummaryError(String(ERROR_PIHOLE_TOO_SMALL_RESPONSE));
        return 8;
    }

    piHoleData.domains_being_blocked = doc["domains_being_blocked"].as<const char*>();
    piHoleData.ads_blocked_today = doc["ads_blocked_today"].as<const char*>();
    piHoleData.ads_percentage_today = doc["ads_percentage_today"].as<const char*>();
    piHoleData.clients_ever_seen = doc["clients_ever_seen"].as<const char*>();
    piHoleData.unique_clients = doc["unique_clients"].as<const char*>();
    piHoleData.piHoleStatus = doc["status"].as<const char*>();

    resetSummaryError();

    return 0;
}

void PiHoleClient::setSummaryError(String message)
{
    SummaryErrorMessage = message;
    piHoleStatus = PIHOLE_UNAVAILABLE;
}

void PiHoleClient::resetSummaryError()
{
    SummaryErrorMessage = "";
    setStatusEnum(piHoleData.piHoleStatus);
}

int8_t PiHoleClient::getTopClientsBlocked(String piholeUri, int port, String apiToken)
{
    resetClientsBlocked();

    if (piholeUri.length() <= 0)
    {
        setTopClienError(String(ERROR_NO_PIHOLE_HOST));
        return 1;
    }

    if (port <= 0)
    {
        setTopClienError(String(ERROR_INVALID_PIHOLE_PORT));
        return 2;
    }

    if (apiToken.length() <= 0)
    {
        setTopClienError(String(ERROR_NO_PIHOLE_API_TOKEN));
        return 3;
    }

    HTTPClient httpClient;
    const String uri = getPiholeUri(piholeUri, port) + String(URI_PIHOLE_API_TOP_BLOCKED) + apiToken;

    httpClient.begin(*_wifiClient, uri);
    const int responseCode = httpClient.GET();

    if (responseCode < 0)
    {
        setTopClienError(String(ERROR_NO_PIHOLE_API_RESPONSE));
        return 4;
    }
    else if (responseCode != 200)
    {
        const String msg = String(ERROR_NO_PIHOLE_API_RESPONSE) + String(responseCode);
        setTopClienError(msg);
        return 5;
    }
    else if (httpClient.getSize() <= 0)
    {
        setTopClienError(String(ERROR_EMPTY_PIHOLE_API_RESPONSE));
        return 6;
    }

    const String response = httpClient.getString();
    httpClient.end();

    const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + 70;
    DynamicJsonDocument doc(bufferSize);

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        setTopClienError(String(ERROR_PIHOLE_JSON_DESERIALIZATION));
        return 7;
    }

    if (measureJson(doc) <= bufferSize / 2)
    {
        setTopClienError(String(ERROR_PIHOLE_TOO_SMALL_RESPONSE));
        return 8;
    }

    JsonObject blocked = doc["top_sources_blocked"].as<JsonObject>();

    int i = 3;
    for (JsonPair p : blocked)
    {   
        blockedClients[3 - i].clientAddress = p.key().c_str();
        blockedClients[3 - i].blockedCount = p.value().as<int>();

        if (i == 0)
            break;
        i--;
    }
    
    resetTopClienError();
    return 0;
}

void PiHoleClient::setTopClienError(String message)
{
    TopCLientError.isPresent = true;
    TopCLientError.message = message;
}

void PiHoleClient::resetTopClienError()
{
    TopCLientError.isPresent = false;
    TopCLientError.message = "";
}

int8_t PiHoleClient::getGraphData(String piholeUri, int port, String apiToken) 
{
    resetBlockedGraphData();

    if (piholeUri.length() <= 0)
    {
        setGraphError(String(ERROR_NO_PIHOLE_HOST));
        return 1;
    }

    if (port <= 0)
    {
        setGraphError(String(ERROR_INVALID_PIHOLE_PORT));
        return 2;
    }

    if (apiToken.length() <= 0)
    {
        setTopClienError(String(ERROR_NO_PIHOLE_API_TOKEN));
        return 3;
    }

    HTTPClient httpClient;
    const String uri = getPiholeUri(piholeUri, port) + String(URI_PIHOLE_API_OVER_TIME_DATA) + apiToken;

    httpClient.begin(*_wifiClient, uri);
    const int responseCode = httpClient.GET();

    if (responseCode < 0)
    {
        setGraphError(String(ERROR_NO_PIHOLE_API_RESPONSE));
        return 4;
    }
    else if (responseCode != 200)
    {
        const String msg = String(ERROR_NO_PIHOLE_API_RESPONSE) + String(responseCode);
        setGraphError(msg);
        return 5;
    }
    else if (httpClient.getSize() <= 0)
    {
        setGraphError(String(ERROR_EMPTY_PIHOLE_API_RESPONSE));
        return 6;
    }

    String response = httpClient.getString();
    response = response.substring(response.indexOf("\"ads_over_time"));
    response = "{" + response;

    char jsonArray [response.length() + 1];
    response.toCharArray(jsonArray, sizeof(jsonArray));

    const size_t bufferSize = 4096;
    DynamicJsonDocument tmp(bufferSize);
    DeserializationError error = deserializeJson(tmp, response);
    if (error)
    {
        setGraphError(String(ERROR_PIHOLE_JSON_DESERIALIZATION));
        return 7;
    }

    JsonObject ads = tmp["ads_over_time"].as<JsonObject>();

    uint8_t i = 0;
    for (JsonPair p : ads)
    {      
        blocked[i] = p.value().as<int>();
        if (blocked[i] > blockedHigh)
            blockedHigh = blocked[i];

        if (i == 143)
            break;
        i++;
    }

    stopIndex = i - graphScreenWidht;
    
    if (stopIndex < 0)
    {
        stopIndex = 0;
    }
    else
    {
        for (uint8_t j = stopIndex; j < 144; j++)
        {
            if (blocked[j] > blockedHigh)
                blockedHigh = blocked[j];
        }
    }

    blockedCount = i;

    resetGraphError();
    return 0;
}

void PiHoleClient::resetClientsBlocked()
{
    for (int i = 0; i < 3; i++)
    {
        blockedClients[i].clientAddress = "";
        blockedClients[i].blockedCount = 0;
    }
}

void PiHoleClient::setGraphError(String message)
{
    GraphError.isPresent = true;
    GraphError.message = message;
}

void PiHoleClient::resetGraphError()
{
    GraphError.isPresent = false;
    GraphError.message = "";
}

void PiHoleClient::setStatusEnum(String status)
{
    const char *tmpStatus = status.c_str();

    if (strcmp(tmpStatus, "enabled")  == 0)
    {
        piHoleStatus = PIHOLE_ENABLED;
    }
    else if (strcmp(tmpStatus, "disabled") == 0)
    {
        piHoleStatus = PIHOLE_DISABLED;
    }
    else
    {
        piHoleStatus = PIHOLE_UNAVAILABLE;
    }
}

void PiHoleClient::resetBlockedGraphData()
{
    for (int i = 0; i < 144; i++)
    {
        blocked[i] = 0;
    }

    blockedHigh = 0;
    blockedCount = 0;
}

String PiHoleClient::getPiholeUri(String host, int port)
{
    return String(URI_SCHEME) + host + ":" + String(port);
}

String PiHoleClient::getDomainsBeingBlocked()
{
    return piHoleData.domains_being_blocked;
}

String PiHoleClient::getAdsBlockedToday()
{
    return piHoleData.ads_blocked_today;
}

String PiHoleClient::getAdsPercentageToday()
{
    return piHoleData.ads_percentage_today;
}

String PiHoleClient::getClientsEverSeen()
{
    return piHoleData.clients_ever_seen;
}

String PiHoleClient::getUniqueClients()
{
    return piHoleData.unique_clients;
}

String PiHoleClient::getPiHoleStatus()
{
    return piHoleData.piHoleStatus;
}

int *PiHoleClient::getBlockedAds()
{
    return blocked;
}

int PiHoleClient::getBlockedHigh()
{
    return blockedHigh;
}

String PiHoleClient::getTopClientBlocked(int i)
{
  return blockedClients[i].clientAddress;
}

int PiHoleClient::getTopClientBlockedCount(int i)
{
  return blockedClients[i].blockedCount;
}

int PiHoleClient::getStopIndex()
{
    return stopIndex;
}

uint8_t PiHoleClient::getBlockedCount()
{
    return blockedCount;
}

uint8_t PiHoleClient::getStatusEnum()
{
    return piHoleStatus;
}

String PiHoleClient::getSummaryErrorMsg()
{
    return SummaryErrorMessage;
}

String PiHoleClient::getTopCLientErrorMsg()
{
    return TopCLientError.message;
}

String PiHoleClient::getGraphErrorMsg()
{
    return GraphError.message;
}
