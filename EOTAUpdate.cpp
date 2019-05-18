#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <EOTAUpdate.h>

EOTAUpdate::EOTAUpdate(
    const String &url,
    const unsigned currentVersion,
    const unsigned long updateIntervalMs)
    :
    _url(url),
    _forceSSL(url.startsWith("https://")),
    _currentVersion(currentVersion),
    _updateIntervalMs(updateIntervalMs),
    _lastUpdateMs(0)
{
}

bool EOTAUpdate::CheckAndUpdate(bool force)
{
    const bool hasEverChecked = _lastUpdateMs != 0;
    const bool lastCheckIsRecent = (millis() - _lastUpdateMs < _updateIntervalMs);
    if (!force && hasEverChecked && lastCheckIsRecent)
    {
        return false;
    }

    log_i("Checking for updates\n");

    _lastUpdateMs = millis();
    String binURL;
    if (GetUpdateFWURL(binURL))
    {
        log_i("Update found. Performing update\n");
        return PerformOTA(binURL);
    }
    return false;
}

bool EOTAUpdate::GetUpdateFWURL(String &binURL)
{
    return GetUpdateFWURL(binURL, _url);
}

bool EOTAUpdate::GetUpdateFWURL(String &binURL, const String &url, const uint16_t retries)
{
    log_d("Fetching OTA config from: %s\n", url);

    if (retries == 0)
    {
        log_e("Too many retries/redirections\n");
        return false;
    }

    bool isSSL = url.startsWith("https");

    if (_forceSSL && !isSSL)
    {
        log_e("Trying to access a non-ssl URL on a secure update checker\n");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        log_d("Wifi not connected\n");
        return false;
    }

    HTTPClient httpClient;
    auto client = WiFiClient();
    if (!httpClient.begin(url))
    {
        log_e("Error initializing client\n");
        return false;
    }

    const char *headerKeys[] = {"Location"};
    httpClient.collectHeaders(headerKeys, 1);
    int httpCode = httpClient.GET();
    switch (httpCode)
    {
    case HTTP_CODE_OK:
        break;
    case HTTP_CODE_MOVED_PERMANENTLY:
        if (httpClient.hasHeader("Location"))
        {
            return GetUpdateFWURL(binURL, httpClient.header("Location"), retries - 1);
        }
    default:
        log_e("[HTTP] [ERROR] [%d] %s\n");
        log_d("Response:\n%s\n",
                httpCode,
                httpClient.errorToString(httpCode),
                httpClient.getString());
        return false;
    }

    auto & payloadStream = httpClient.getStream();
    binURL = payloadStream.readStringUntil('\n');
    unsigned newVersionNumber = payloadStream.readStringUntil('\n').toInt();
    String newVersionString = payloadStream.readStringUntil('\n');
    httpClient.end();

    if (binURL.length() == 0)
    {
        log_e("Error parsing remote path of new binary\n");
        return false;
    }

    if (newVersionNumber == 0)
    {
        log_e("Error parsing version number\n");
        return false;
    }

    log_d("Fetched update information:\n");
    log_d(".bin url:           %s\n",       binURL);
    log_d("Current version:    %u\n",       _currentVersion);
    log_d("Published version:  [%u] %s\n",  newVersionNumber, newVersionString);
    log_d("Update available:   %s\n",       (newVersionNumber > _currentVersion) ? "YES" : "NO");

    return newVersionNumber > _currentVersion;
}

bool EOTAUpdate::PerformOTA(String &binURL)
{
    log_d("Fetching OTA from: %s\n", binURL);

    if (binURL.length() == 0)
    {
        return false;
    }

    bool isSSL = binURL.startsWith("https");
    if (_forceSSL && !isSSL)
    {
        log_e("Trying to access a non-ssl URL on a secure update checker\n");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        log_d("Wifi not connected\n");
        return false;
    }

    HTTPClient httpClient;
    if (!httpClient.begin(binURL))
    {
        log_e("Error initializing client\n");
        return false;
    }

    const auto httpCode = httpClient.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        log_e("[HTTP] [ERROR] [%d] %s\n");
        log_d("Response:\n%s\n",
                httpCode,
                httpClient.errorToString(httpCode),
                httpClient.getString());
        return false;
    }

    const auto payloadSize = httpClient.getSize();
    auto & payloadStream = httpClient.getStream();
    const bool canBegin = Update.begin(payloadSize);

    if (payloadSize <= 0)
    {
        log_e("Fetched binary has 0 size\n");
        return false;
    }

    if (!canBegin)
    {
        log_e("Not enough space to begin OTA\n");
        return false;
    }

    const auto written = Update.writeStream(payloadStream);
    if (written != payloadSize)
    {
        log_e("Error. Written %s out of %s\n", String(written), String(payloadSize));
        return false;
    }

    if (!Update.end())
    {
        log_e("Error Occurred: %s\n", String(Update.getError()));
        return false;
    }

    if (!Update.isFinished())
    {
        log_e("Undefined OTA update error\n");
        return false;
    }

    log_i("Update completed. Rebooting\n");
    ESP.restart();
    return true;
}
