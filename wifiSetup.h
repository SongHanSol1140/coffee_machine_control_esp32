// wifiSetup.h
#ifndef WIFISETUP_H
#define WIFISETUP_H

#include <IPAddress.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Preferences.h>   // ★추가

extern WiFiClient net;
extern AsyncWebServer server;

void wifiSetup();
void startWebServer();

// ★ NVS에서 설정 로드/저장
void loadSettings();
void saveSetting(const String& key, int value);
void saveSetting(const String& key, float value);
void saveSetting(const String& key, double value);

#endif
