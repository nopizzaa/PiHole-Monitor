#include "Settings.h"

// DFRobot_SHT20 sht20;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

EasyButton button(BUTTON_PIN);

WiFiClient wifiClient;
OWMClient weather(wifiClient);
PiHoleClient piholeClient(wifiClient, SCREEN_WIDHT);
PubSubClient mqttClient(wifiClient);

AsyncWebServer server(WEBSERVER_PORT);

SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SCL_PIN);
OLEDDisplayUi ui(&display);

Sun sunsetSunrise;

Thermistor *thermistor = NULL;

void drawScreen1(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawScreen2(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawScreen3(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void graphScreen(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawClientsBlocked(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawDomainsOnList(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state);
void drawScreen4(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawScreen5(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawNoPihole(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawOWHoffline(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);

OverlayCallback overlays[] = {drawHeaderOverlay};

void mqttCallback(char *topic, uint8_t *payload, unsigned int length) {}

FrameCallback frames[8];
FrameCallback framesPiholeOffline[3];

void uiNextFrame()
{
    ui.nextFrame();
}

void uiPreviousFrame()
{
    ui.previousFrame();
}

void uiPauseStartAutoTransition()
{
    if (autoTransition)
    {
        ui.disableAutoTransition();
        autoTransition = false;
    }
    else
    {
        ui.enableAutoTransition();
        autoTransition = true;
    }
}

void setupDisplayFrames()
{
    frames[0] = drawScreen1;
    frames[1] = drawScreen2;
    frames[2] = drawScreen3;
    frames[3] = drawDomainsOnList;
    frames[4] = graphScreen;
    frames[5] = drawClientsBlocked;
    frames[6] = drawScreen4;
    frames[7] = drawScreen5;

    framesPiholeOffline[0] = drawNoPihole;
    framesPiholeOffline[1] = drawScreen4;
    framesPiholeOffline[2] = drawScreen5;
}

void configModeCallback(AsyncWiFiManager *myWiFiManager)
{
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println("Wifi Manager");
    Serial.println("Please connect to AP");
    Serial.println(myWiFiManager->getConfigPortalSSID());
    Serial.println("To setup Wifi Configuration");

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 0, "Wifi Manager");
    display.drawString(64, 10, "Please connect to AP");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 23, myWiFiManager->getConfigPortalSSID());
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 42, "To setup Wifi connection");
    display.display();
}

String getValueOfString(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void updateWeather()
{
    Serial.println("Getting Weather Data...");
    int8_t errorCode = -1;

    for (uint8_t i = 0; i < 3; ++i)
    {
        errorCode = weather.getWeather(Config::weatherCityId, Config::weatherApiKey);
        if (errorCode == 0)
            break;
        delay(500);
    }

    if (weather.getErrorIsPresent() != weatherErrorWasPresent)
    {
        if (weather.getErrorIsPresent())
        {
            frames[7] = drawOWHoffline;
            framesPiholeOffline[2] = drawOWHoffline;
        }
        else
        {
            frames[7] = drawScreen5;
            framesPiholeOffline[2] = drawScreen5;
        }

        weatherErrorWasPresent = weather.getErrorIsPresent();
    }

    Serial.println("Status Code: " + String(errorCode) + " Message: " + weather.getErrorMsg());
}

void updatePiHoleSummary()
{
    int8_t errorCode = -1;
    for (uint8_t i = 0; i < 3; ++i)
    {
        errorCode = piholeClient.getPiHoleSummary(Config::piholeUri, Config::piholePort, Config::piholeApiToken);
        if (errorCode == 0)
            break;
        delay(500);
    }

    const boolean errorIsNotPresent = errorCode == 0;

    if (errorIsNotPresent != piholeErrorWasNotPresent)
    {
        if (errorIsNotPresent)
        {
            ui.setFrames(frames, 8);
        }
        else
        {
            ui.setFrames(framesPiholeOffline, 3);
        }

        piholeErrorWasNotPresent = errorIsNotPresent;
    }

    if (!errorIsNotPresent)
        return;

    for (uint8_t i = 0; i < 3; ++i)
    {
        if (piholeClient.getTopClientsBlocked(Config::piholeUri, Config::piholePort, Config::piholeApiToken) == 0)
            break;
        delay(500);
    }
}

void updatePiHoleGraph()
{
    for (uint8_t i = 0; i < 3; ++i)
    {
        if (piholeClient.getGraphData(Config::piholeUri, Config::piholePort, Config::piholeApiToken) == 0)
            break;
        delay(500);
    }
}

uint8_t getWifiQuality()
{
    const int dbm = WiFi.RSSI();

    if (dbm <= -100)
        return 0;
    else if (dbm >= -50)
        return 100;
    else
        return 2 * (dbm + 100);
}

void drawWifiQuality(OLEDDisplay *display)
{
    const int8_t quality = getWifiQuality();
    for (int8_t i = 0; i < 4; i++)
    {
        for (int8_t j = 0; j < 3 * (i + 2); j++)
        {
            if (quality > i * 25 || j == 0)
                display->setPixel(114 + 4 * i, 63 - j);
        }
    }
}

String formatTime(unsigned int time)
{
    return time < 10 ? "0" + String(time) : String(time);
}

void autoBrightness(unsigned int current, unsigned int sunset, unsigned int sunrise)
{
    if ((current < sunset && current >= sunrise) && nightTimeIsActive)
    {
        // day
        display.setBrightness(Config::displayBrightness);
        nightTimeIsActive = false;
    }
    else if (!(current < sunset && current >= sunrise) && !nightTimeIsActive)
    {
        // night
        display.setBrightness(Config::nightTimeBrightness);
        nightTimeIsActive = true;
    }
}

unsigned int calcualteEpochSecunds(unsigned long time)
{
    return (unsigned int)round(time + 86400L) % 86400L;
}

void handleNightTime()
{
    if (!Config::nightTimeEnabled)
        return;

    const unsigned long currentEpochTime = timeClient.getEpochTime();
    const unsigned int epochSecunds = calcualteEpochSecunds(currentEpochTime);

    if (Config::nightTimeOnAuto)
    {
        autoBrightness(epochSecunds,
                       calcualteEpochSecunds(sunsetSunrise.getSet(currentEpochTime) + Config::ntpTimeOffset),
                       calcualteEpochSecunds(sunsetSunrise.getRise(currentEpochTime) + Config::ntpTimeOffset));
    }
    else
    {
        autoBrightness(epochSecunds, Config::nightTimeOn, Config::nightTimeOff);
    }
}

#pragma region display_farmes

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state)
{
    display->setFont(ArialMT_Plain_16);
    const String displayTime = formatTime(timeClient.getHours()) +
                               (timeClient.getSeconds() % 2 == 0 ? ":" : " ") +
                               formatTime(timeClient.getMinutes());

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 48, displayTime);

    if (!autoTransition)
    {
        display->drawXbm(44, 51, 10, 12, Pause_Icon_Bits);
    }
    else if (nightTimeIsActive)
    {
        display->drawXbm(44, 51, 10, 12, Moon_Icon_Bits);
    }

    display->setFont(ArialMT_Plain_16);
    display->setTextAlignment(TEXT_ALIGN_LEFT);

    switch (piholeClient.getStatusEnum())
    {
    case 0:
    {
        const String percent = String(piholeClient.getAdsPercentageToday()) + "%";
        display->drawString(60, 48, percent);
        break;
    }
    case 1:
        display->drawString(66, 48, "OFF");
        break;

    default:
        display->drawString(55, 48, "DOWN");
        break;
    }

    if (mqttClient.connected())
        display->drawXbm(105, 51, 11, 13, Mqtt_Icon_Bits);

    drawWifiQuality(display);
}

void drawScreen1(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    ui.enableIndicator();
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 7 + y, "Queries Blocked");
    display->setFont(ArialMT_Plain_24);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(64 + x, 21 + y, piholeClient.getAdsPercentageToday() + "%");
}

void drawScreen2(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 7 + y, "Total Clients");
    display->setFont(ArialMT_Plain_24);
    display->drawString(64 + x, 21 + y, piholeClient.getUniqueClients() + " / " + piholeClient.getClientsEverSeen());
}

void drawScreen3(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 7 + y, "Total Blocked");
    display->setFont(ArialMT_Plain_24);
    display->drawString(64 + x, 21 + y, piholeClient.getAdsBlockedToday());
}

void drawScreen4(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    ui.enableIndicator();
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 7 + y, "Temperature");
    display->setFont(ArialMT_Plain_24);
    display->drawString(64 + x, 21 + y, String(round(temperature * 10) / 10, 1) + "°C");
}

void drawScreen5(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    ui.disableIndicator();
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_24);
    display->drawString(85 + x, 10 + y, weather.getFromatedTemperature());
    display->setFont(ArialMT_Plain_10);
    const String desc = weather.getDescription();
    display->drawString(85 + x, 0 + y, display->getStringWidth(desc) > 82 ? weather.getCondition() : desc);
    display->drawString(85 + x, 34 + y, weather.getFromatedFeelsLike() + " - " + weather.getFromatedWindSpeed());
    display->setFont((const uint8_t *)Meteocons_Plain_42);
    display->drawString(29 + x, 2 + y, weather.getWeatherIconScreen());
}

void drawClientsBlocked(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    int16_t row = 0;

    for (int i = 0; i < 3; i++)
    {
        if (piholeClient.getTopClientBlocked(i) != "")
        {
            const String blockedCount = " (" + String(piholeClient.getTopClientBlockedCount(i)) + ")";
            const String host = piholeClient.getTopClientBlocked(i) + blockedCount;

            if (display->getStringWidth(host) < 128)
            {
                display->drawString(64 + x, row + 8 + y, host);
            }
            else
            {
                const String hostIp = getValueOfString(piholeClient.getTopClientBlocked(i), '|', 0);
                display->drawString(64 + x, row + 8 + y, hostIp + blockedCount);
            }
        }
        row += 11;
    }
}

void graphScreen(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    ui.disableIndicator();
    const uint8_t count = piholeClient.getBlockedCount();

    if (count > 0)
    {
        uint8_t row = 127;
        int yHeight = 0;
        const int stopIndex = piholeClient.getStopIndex();
        const int high = piholeClient.getBlockedHigh();

        for (int i = count - 1; i >= stopIndex; i--)
        {
            yHeight = map(piholeClient.getBlockedAds()[i], high, 0, 0, 45);
            display->drawLine(row + x, yHeight + y, row + x, 45 + y);

            if (row == 0)
                break;
            row--;
        }
    }
    else
    {
        ui.enableIndicator();
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        display->setFont(ArialMT_Plain_16);
        display->drawString(64 + x, 7 + y, "Graphicon");
        display->setFont(ArialMT_Plain_24);
        display->drawString(64 + x, 21 + y, "No Data");
    }
}

void drawDomainsOnList(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 7 + y, "Domains List");
    display->setFont(ArialMT_Plain_24);
    display->drawString(64 + x, 21 + y, piholeClient.getDomainsBeingBlocked());
}

void drawNoPihole(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    ui.enableIndicator();
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 7 + y, "PiHole Is");
    display->setFont(ArialMT_Plain_24);
    display->drawString(64 + x, 23 + y, "OFFLINE");
}

void drawOWHoffline(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    ui.disableIndicator();
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 0 + y, "Weather Service");
    display->setFont(ArialMT_Plain_24);
    display->drawString(64 + x, 16 + y, "OFFLINE");
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 40 + y, weather.getErrorMsg());
}

#pragma endregion

/*
void updateSht20State()
{
    humidity = sht20.readHumidity();
    temperature = sht20.readTemperature();
}
*/

void publishStateViaMqtt()
{
    DynamicJsonDocument wifiJson(192);
    DynamicJsonDocument stateJson(894);
    char payload[256];

    // stateJson["sht20_humidity"] = humidity;
    stateJson["temperature"] = temperature;

    wifiJson["ssid"] = WiFi.SSID();
    wifiJson["ip"] = WiFi.localIP().toString();
    wifiJson["rssi"] = WiFi.RSSI();

    stateJson["wifi"] = wifiJson.as<JsonObject>();

    serializeJson(stateJson, payload);
    mqttClient.publish(&MQTT_TOPIC_STATE[0], &payload[0], true);
}

void publishAutoConfig()
{
    char mqttPayload[2048];
    DynamicJsonDocument device(256);
    DynamicJsonDocument autoconfPayload(1024);
    StaticJsonDocument<64> identifiersDoc;
    JsonArray identifiers = identifiersDoc.to<JsonArray>();

    identifiers.add(identifier);

    device["identifiers"] = identifiers;
    device["manufacturer"] = "unknown";
    device["model"] = "PIHOLE-MONITOR";
    device["name"] = identifier;
    device["sw_version"] = "2022.12.0";

    autoconfPayload["device"] = device.as<JsonObject>();
    autoconfPayload["availability_topic"] = MQTT_TOPIC_AVAILABILITY;
    autoconfPayload["state_topic"] = MQTT_TOPIC_STATE;
    autoconfPayload["name"] = identifier + String(" WiFi");
    autoconfPayload["value_template"] = "{{value_json.wifi.rssi}}";
    autoconfPayload["unique_id"] = identifier + String("_wifi");
    autoconfPayload["unit_of_measurement"] = "dBm";
    autoconfPayload["json_attributes_topic"] = MQTT_TOPIC_STATE;
    autoconfPayload["json_attributes_template"] = "{\"ssid\": \"{{value_json.wifi.ssid}}\", \"ip\": \"{{value_json.wifi.ip}}\"}";
    autoconfPayload["icon"] = "mdi:wifi";

    serializeJson(autoconfPayload, mqttPayload);
    mqttClient.publish(&MQTT_TOPIC_AUTOCONF_WIFI_SENSOR[0], &mqttPayload[0], true);

    autoconfPayload.clear();

    /*
    autoconfPayload["device"] = device.as<JsonObject>();
    autoconfPayload["availability_topic"] = MQTT_TOPIC_AVAILABILITY;
    autoconfPayload["state_topic"] = MQTT_TOPIC_STATE;
    autoconfPayload["name"] = identifier + String(" Humidity");
    autoconfPayload["unit_of_measurement"] = "% RH";
    autoconfPayload["value_template"] = "{{(value_json.sht20_humidity)|round(1)}}";
    autoconfPayload["unique_id"] = identifier + String("_sht20_humidity");
    autoconfPayload["icon"] = "mdi:water-percent";

    serializeJson(autoconfPayload, mqttPayload);
    mqttClient.publish(&MQTT_TOPIC_AUTOCONF_SHT20_HUMIDITY_SENSOR[0], &mqttPayload[0], true);

    autoconfPayload.clear();
    */

    autoconfPayload["device"] = device.as<JsonObject>();
    autoconfPayload["availability_topic"] = MQTT_TOPIC_AVAILABILITY;
    autoconfPayload["state_topic"] = MQTT_TOPIC_STATE;
    autoconfPayload["name"] = identifier + String(" Temperature");
    autoconfPayload["unit_of_measurement"] = "˚C";
    autoconfPayload["value_template"] = "{{(value_json.temperature)|round(1)}}";
    autoconfPayload["unique_id"] = identifier + String("_thermistor_temperature");
    autoconfPayload["icon"] = "mdi:thermometer";

    serializeJson(autoconfPayload, mqttPayload);
    mqttClient.publish(&MQTT_TOPIC_AUTOCONF_TEMPERATURE_SENSOR[0], &mqttPayload[0], true);

    autoconfPayload.clear();
}

void mqttReconnect()
{
    for (uint8_t attempt = 0; attempt < 3; ++attempt)
    {
        if (mqttClient.connect(identifier, Config::mqttUsername, Config::mqttPassword, MQTT_TOPIC_AVAILABILITY, 1, true, AVAILABILITY_OFFLINE))
        {
            mqttClient.publish(MQTT_TOPIC_AVAILABILITY, AVAILABILITY_ONLINE, true);
            publishAutoConfig();

            // Make sure to subscribe after polling the status so that we never execute commands with the default data
            mqttClient.subscribe(MQTT_TOPIC_COMMAND);
            break;
        }
        delay(500);
    }
}

void initSettings()
{
    Config::loadConfig();

    timeClient.setTimeOffset(Config::ntpTimeOffset);
    if (strlen(Config::ntpPoolServerName) <= 0)
        timeClient.setPoolServerName(Config::ntpPoolServerName);

    if (strlen(Config::mqttServer) <= 0 || strlen(Config::mqttUsername) <= 0 || strlen(Config::mqttPassword) <= 0)
    {
        mqttServerEnabled = false;
    }
    else
    {
        mqttClient.setServer(Config::mqttServer, MQTT_PORT);
        mqttServerEnabled = true;
    }

    sunsetSunrise.setCoordinates(Config::lat, Config::lon);

    setupDisplayFrames();
}

void setupOTA()
{
    ArduinoOTA.onStart([]()
                       { Serial.println("Start"); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
        Serial.printf("Error[%u]: ", error);

        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        } });

    ArduinoOTA.setHostname(identifier);

    // This is less of a security measure and more a accidential flash prevention
    ArduinoOTA.setPassword(identifier);
    ArduinoOTA.begin();
}

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);

    Serial.printf("Core Version: %s\n", ESP.getCoreVersion().c_str());
    Serial.printf("Boot Version: %u\n", ESP.getBootVersion());
    Serial.printf("Boot Mode: %u\n", ESP.getBootMode());
    Serial.printf("CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Reset reason: %s\n", ESP.getResetReason().c_str());

    // sht20.initSHT20();
    // sht20.checkSHT20();

    Thermistor *originThermistor = new NTC_Thermistor(
        SENSOR_PIN,
        REFERENCE_RESISTANCE,
        NOMINAL_RESISTANCE,
        NOMINAL_TEMPERATURE,
        B_VALUE);
    thermistor = new AverageThermistor(
        originThermistor,
        READINGS_NUMBER,
        DELAY_TIME);

    if (!LittleFS.begin())
    {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    DNSServer dns;
    AsyncWiFiManager wifiManager(&server, &dns);

    snprintf(identifier, sizeof(identifier), HOSTNAME, ESP.getChipId());
    snprintf(MQTT_TOPIC_AVAILABILITY, 127, "%s/%s/status", FIRMWARE_PREFIX, identifier);
    snprintf(MQTT_TOPIC_STATE, 127, "%s/%s/state", FIRMWARE_PREFIX, identifier);
    snprintf(MQTT_TOPIC_COMMAND, 127, "%s/%s/command", FIRMWARE_PREFIX, identifier);
    snprintf(MQTT_TOPIC_AUTOCONF_TEMPERATURE_SENSOR, 127, "homeassistant/sensor/%s/%s_thermistor_temperature/config", FIRMWARE_PREFIX, identifier);
    // snprintf(MQTT_TOPIC_AUTOCONF_SHT20_HUMIDITY_SENSOR, 127, "homeassistant/sensor/%s/%s_sht20_humidity/config", FIRMWARE_PREFIX, identifier);
    snprintf(MQTT_TOPIC_AUTOCONF_WIFI_SENSOR, 127, "homeassistant/sensor/%s/%s_wifi/config", FIRMWARE_PREFIX, identifier);

    display.init();
    display.invertDisplay();

    // display.flipScreenVertically();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 12, "Welcomme!");
    display.drawString(64, 40, identifier);
    display.display();

    WiFi.hostname(identifier);
    wifiManager.setAPCallback(configModeCallback);
    if (!wifiManager.autoConnect(identifier))
    {
        delay(3000);
        WiFi.disconnect(true);
        ESP.reset();
        delay(3000);
    }

    initSettings();
    setupOTA();

    timeClient.setUpdateInterval(NTP_UPDATE_INTERVAL);
    timeClient.begin();

    ui.setFrameAnimation(SLIDE_LEFT);
    ui.setTargetFPS(TARGET_FPS);
    ui.setIndicatorPosition(TOP);
    ui.setFrames(frames, 8);
    ui.setOverlays(overlays, 1);
    ui.init();
    // display.flipScreenVertically();

    button.begin();
    button.onPressed(uiNextFrame);
    button.onPressedFor(BTN_LONG_PRESSED, uiPauseStartAutoTransition);

    // updateSht20State();

    temperature = thermistor->readCelsius();

    mqttClient.setKeepAlive(10);
    mqttClient.setBufferSize(2048);
    mqttClient.setCallback(mqttCallback);

    timeClient.forceUpdate();

    if (mqttServerEnabled)
        mqttReconnect();

    updateWeather();
    handleNightTime();
    updatePiHoleSummary();
    updatePiHoleGraph();

    piholeErrorWasNotPresent = !piholeErrorWasNotPresent;
    weatherErrorWasPresent = !weatherErrorWasPresent;

    server.serveStatic("/bootstrap.min.css", LittleFS, "/bootstrap.min.css");
    server.serveStatic("/style.css", LittleFS, "/style.css");
    server.serveStatic("/bootstrap.bundle.min.js", LittleFS, "/bootstrap.bundle.min.js");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/index.html", String()); });
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        const unsigned long currentEpochTime = timeClient.getEpochTime();
        DynamicJsonDocument json(2048);
        json["time_local"] = timeClient.getFormattedTime();
        json["ssid"] = WiFi.SSID();
        json["wifi_quality"] = getWifiQuality();
        json["ip"] = WiFi.localIP().toString();
        json["hostname"] = WiFi.getHostname();
        // json["sht20_humidity"] = humidity;
        json["temperature"] = temperature;
        json["mqtt_is_enabled"] = mqttServerEnabled;
        json["mqtt_is_conected"] = mqttClient.connected();
        json["coord_lat"] = Config::lat;
        json["coord_lon"] = Config::lon;
        json["sunset_local_unix"] = sunsetSunrise.getSet(currentEpochTime) + Config::ntpTimeOffset;
        json["sunrise_local_unix"] = sunsetSunrise.getRise(currentEpochTime) + Config::ntpTimeOffset;
        json["ads_percentage_today"] = piholeClient.getAdsPercentageToday();
        json["domains_being_blocked"] = piholeClient.getDomainsBeingBlocked();
        json["ads_blocked_today"] = piholeClient.getAdsBlockedToday();
        json["clients_ever_seen"] = piholeClient.getClientsEverSeen();
        json["unique_clients"] = piholeClient.getUniqueClients();
        json["pihole_status"] = piholeClient.getPiHoleStatus();
        json["weather_city"] = weather.getCity();
        json["weather_country"] = weather.getCountry();
        json["weather_temp"] = weather.getTemperature();
        json["weather_humidity"] = weather.getHumidity();
        json["weather_feelslike"] = weather.getTempFeelslike();
        json["weather_condition"] = weather.getCondition();
        json["weather_desc"] = weather.getDescription();
        json["weather_wind"] = weather.getWind();
        json["weather_error_msg"] = weather.getErrorMsg();
        json["pihole_sum_error_msg"] = piholeClient.getSummaryErrorMsg();
        json["pihole_top_error_msg"] = piholeClient.getTopCLientErrorMsg();
        serializeJson(json, *response);
        request->send(response); });
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Page not found"); });

    server.begin();
}

void loop()
{
    ArduinoOTA.handle();
    button.read();
    timeClient.update();
    mqttClient.loop();

    const unsigned long currentMillis = millis();

    if (currentMillis - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL)
    {
        lastWeatherUpdate = currentMillis;
        updateWeather();
    }
    if (currentMillis - lastPiholeUpdateMqttReconet >= UPDATE_PIHOLESUM_MQTT_RECONET_INTERVAL)
    {
        lastPiholeUpdateMqttReconet = currentMillis;
        handleNightTime();
        updatePiHoleSummary();
        if (!mqttClient.connected() && mqttServerEnabled)
            mqttReconnect();
    }
    if (currentMillis - lastSensorMqttUpdate >= SENSOR_MQTT_UPDATE_INTERVAL)
    {
        lastSensorMqttUpdate = currentMillis;
        temperature = thermistor->readCelsius();
        delay(100);
        if (mqttServerEnabled)
            publishStateViaMqtt();
    }
    if (!piholeErrorWasNotPresent && currentMillis - lastPiholeGraphUpdate >= UPDATE_PIHOLE_GRAPH_INTERVAL)
    {
        lastPiholeGraphUpdate = currentMillis;
        updatePiHoleGraph();
    }

    ui.update();
}