#include "OWMClient.h"

OWMClient::OWMClient(WiFiClient &wifiClient) 
{
    _wifiClient = &wifiClient;
}

int8_t OWMClient::getWeather(unsigned int cityId, String apiKey)
{
    if (apiKey.length() <= 0)
    {
        setErrorMsg(String(ERROR_NO_API_KEY));
        return 1;
    }

    const String uri = String(URI_OWM) + String(cityId) + String(URI_OWM_APIKEY) + apiKey + String(URI_OWM_LANG);

    HTTPClient httpClient;
    httpClient.begin(*_wifiClient, uri);
    const int responseCode = httpClient.GET();

    if (responseCode <= 0)
    {
        setErrorMsg(String(ERROR_NO_RESPONSE));
        return 2;
    }
    else if (responseCode != 200)
    {
        const String msg =  String(ERROR_UNEXPECTED_RESPONSE) + String(responseCode);
        setErrorMsg(msg);
        return 3;
    }
    else if (httpClient.getSize() <= 0)
    {
        setErrorMsg(String(ERROR_EMPTY_RESPONSE));
        return 4;
    }

    const String response = httpClient.getString();
    httpClient.end();

    const size_t bufferSize = 1792;
    DynamicJsonDocument doc(bufferSize);

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        setErrorMsg(String(ERROR_JSON_DESERIALIZATION));
        return 5;
    }

    if (measureJson(doc) <= 150)
    {
        const String msg = String(ERROR_NO_VALID_DATA) + 
                           "Size: " + String(measureJson(doc)) + 
                           " Msg: " + doc["message"].as<const char*>();
        setErrorMsg(msg);
        return 6;
    }

    weather.city        = doc["list"][0]["name"].as<const char*>();
    weather.country     = doc["list"][0]["sys"]["country"].as<const char*>();
    weather.humidity    = doc["list"][0]["main"]["humidity"].as<int>();
    weather.condition   = doc["list"][0]["weather"][0]["main"].as<const char*>();
    weather.description = doc["list"][0]["weather"][0]["description"].as<const char*>();
    weather.webIcon     = doc["list"][0]["weather"][0]["icon"].as<const char*>();

    const float tmpWind = doc["list"][0]["wind"]["speed"].as<float>();
    weather.wind = tmpWind;
    weather.formatedWind = formatWindSpeed(tmpWind);

    const int tmpWeatherId = doc["list"][0]["weather"][0]["id"].as<int>();
    weather.weatherId = tmpWeatherId;
    weather.icon = getWeatherIcon(tmpWeatherId);

    const float tmpTemp = doc["list"][0]["main"]["temp"].as<float>();
    weather.temp = tmpTemp;
    weather.formatedTemp = formatTemperature(tmpTemp);

    const float tmpFeelsLike = doc["list"][0]["main"]["feels_like"].as<float>();
    weather.feelsLike = tmpFeelsLike;
    weather.formatedFeelsLike = formatTemperature(tmpFeelsLike);

    resetErrorMsg();
    return 0;
}

String OWMClient::formatWindSpeed(float windSpeed)
{
    String wind = String(getWindSpeedInKph(windSpeed));
    return wind + " kph";
}

String OWMClient::formatTemperature(float temp)
{
    return String(roundToInt(temp)) + "°C";
}

void OWMClient::setErrorMsg(String msg)
{
    errorIsPresent = true;
    errorMsg = msg;
}

void OWMClient::resetErrorMsg()
{
    errorIsPresent = false;
    errorMsg = "";
}

int OWMClient::roundToInt(float value)
{
    return (int)round(value);
}

int OWMClient::getWindSpeedInKph(float wind) const
{
    return (int)round(wind * 3.6f);
}

String OWMClient::getWeatherIcon(int id) const
{
    String W = ")";

    switch (id)
    {
    case 800:
        W = "B";
        break;
    case 801:
        W = "Y";
        break;
    case 802:
        W = "H";
        break;
    case 803:
        W = "H";
        break;
    case 804:
        W = "Y";
        break;

    case 200:
        W = "0";
        break;
    case 201:
        W = "0";
        break;
    case 202:
        W = "0";
        break;
    case 210:
        W = "0";
        break;
    case 211:
        W = "0";
        break;
    case 212:
        W = "0";
        break;
    case 221:
        W = "0";
        break;
    case 230:
        W = "0";
        break;
    case 231:
        W = "0";
        break;
    case 232:
        W = "0";
        break;

    case 300:
        W = "R";
        break;
    case 301:
        W = "R";
        break;
    case 302:
        W = "R";
        break;
    case 310:
        W = "R";
        break;
    case 311:
        W = "R";
        break;
    case 312:
        W = "R";
        break;
    case 313:
        W = "R";
        break;
    case 314:
        W = "R";
        break;
    case 321:
        W = "R";
        break;

    case 500:
        W = "R";
        break;
    case 501:
        W = "R";
        break;
    case 502:
        W = "R";
        break;
    case 503:
        W = "R";
        break;
    case 504:
        W = "R";
        break;
    case 511:
        W = "R";
        break;
    case 520:
        W = "R";
        break;
    case 521:
        W = "R";
        break;
    case 522:
        W = "R";
        break;
    case 531:
        W = "R";
        break;

    case 600:
        W = "W";
        break;
    case 601:
        W = "W";
        break;
    case 602:
        W = "W";
        break;
    case 611:
        W = "W";
        break;
    case 612:
        W = "W";
        break;
    case 615:
        W = "W";
        break;
    case 616:
        W = "W";
        break;
    case 620:
        W = "W";
        break;
    case 621:
        W = "W";
        break;
    case 622:
        W = "W";
        break;

    case 701:
        W = "M";
        break;
    case 711:
        W = "M";
        break;
    case 721:
        W = "M";
        break;
    case 731:
        W = "M";
        break;
    case 741:
        W = "M";
        break;
    case 751:
        W = "M";
        break;
    case 761:
        W = "M";
        break;
    case 762:
        W = "M";
        break;
    case 771:
        W = "M";
        break;
    case 781:
        W = "M";
        break;

    default:
        break;
    }
    return W;
}

String OWMClient::getErrorMsg()
{
    return errorMsg;
}

boolean OWMClient::getErrorIsPresent()
{
    return errorIsPresent;
}

String OWMClient::getCity()
{
    return weather.city;
}

String OWMClient::getCountry()
{
    return weather.country;
}

float OWMClient::getTemperature() 
{
    return weather.temp;
}

String OWMClient::getFromatedTemperature() 
{
    return weather.formatedTemp;
}

unsigned int OWMClient::getHumidity() 
{
    return weather.humidity;
}

String OWMClient::getFromatedFeelsLike() 
{
    return weather.formatedFeelsLike;
}

String OWMClient::getFromatedWindSpeed() 
{
    return weather.formatedWind;
}

String OWMClient::getDescription()
{
    return weather.description;
}

String OWMClient::getCondition()
{
    return weather.condition;
}

String OWMClient::getWeatherIconScreen()
{
    return weather.icon;
}

String OWMClient::getWeatherWebIcon()
{
    return weather.webIcon;
}

float OWMClient::getTempFeelslike()
{
    return weather.feelsLike;
}

float OWMClient::getWind()
{
    return weather.wind;
}
