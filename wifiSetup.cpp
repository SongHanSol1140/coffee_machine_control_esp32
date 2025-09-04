// wifiSetup.cpp
#include "wifiSetup.h"
#include "variables.h"

// 와이파이
const char* wifi_ssid = "NNX2-2.4G";
const char* wifi_password = "$@43skshslrtm";
const IPAddress wifiIP(192, 168, 0, 231);  // 접속할 고정 IP 주소
const IPAddress gateway(192, 168, 0, 1);   // 게이트웨이 주소
const IPAddress subnet(255, 255, 255, 0);  // 서브넷 마스크
const IPAddress dns(192, 168, 0, 1);

WiFiClient net;
AsyncWebServer server(80);


// 현재 4개 상태를 JSON으로 묶어 반환
static String buildStateJson() {
  String s = "{";
  s += "\"isRunning\":";     s += (isRunning ? "true" : "false");     s += ",";
  s += "\"emergencyStop\":"; s += (emergencyStop ? "true" : "false"); s += ",";
  s += "\"isHot\":";         s += (isHot ? "true" : "false");         s += ",";
  s += "\"isCold\":";        s += (isCold ? "true" : "false");
  s += "}";
  return s;
}

void wifiSetup() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.println("connecting wifi...");
      delay(1000);
    }
    Serial.print("Wifi IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("WIFI connected!");
  }
  delay(100);
}

void startWebServer() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }
  Serial.println("LittleFS mounted");

  // 정적 파일 제공
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // ---- API ----
  // 0) 상태 조회 (클라이언트가 0.5초마다 폴링)
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildStateJson());
  });

  // 1) System On/Off 토글
  server.on("/api/isRunning/toggle", HTTP_POST, [](AsyncWebServerRequest* req) {
    isRunning = !isRunning;
    req->send(200, "application/json", buildStateJson());
  });

  // 2) 긴급정지(한 번 누르면 true로 세팅, 시리얼 로그 출력)
  server.on("/api/emergencyStop", HTTP_POST, [](AsyncWebServerRequest* req) {
    emergencyStop = true;
    Serial.println("EmergencyStop");
    req->send(200, "application/json", buildStateJson());
  });

  // 3) HOT 설정 (isHot = true, isCold = false)
  server.on("/api/hot", HTTP_POST, [](AsyncWebServerRequest* req) {
    isHot = true;
    isCold = false;
    req->send(200, "application/json", buildStateJson());
  });

  // 4) COLD 설정 (isCold = true, isHot = false)
  server.on("/api/cold", HTTP_POST, [](AsyncWebServerRequest* req) {
    isCold = true;
    isHot = false;
    req->send(200, "application/json", buildStateJson());
  });

  server.begin();
  Serial.println("HTTP server started");
}