// wifiSetup.h
#ifndef WIFISETUP_H
#define WIFISETUP_H
// 와이파이
#include <IPAddress.h>
#include <ArduinoJson.h>
#include <WiFi.h>
// 웹페이지 제공
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
// 와이파이 정보
extern const char* wifi_ssid;
extern const char* wifi_password;
extern const IPAddress wifiIP;    // 고정 IP 주소
extern const IPAddress gateway;   // 게이트웨이 주소
extern const IPAddress subnet;    // 서브넷 마스크
extern const IPAddress dns;    // 서브넷 마스크

extern WiFiClient net;
extern AsyncWebServer server;

void wifiSetup();
void startWebServer();
#endif // WIFISETUP_H