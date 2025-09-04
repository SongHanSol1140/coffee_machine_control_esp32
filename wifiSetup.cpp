#include "wifiSetup.h"
#include "variables.h"

const char* wifi_ssid = "NNX2-2.4G";
const char* wifi_password = "$@43skshslrtm";
const IPAddress wifiIP(192,168,0,231);
const IPAddress gateway(192,168,0,1);
const IPAddress subnet(255,255,255,0);
const IPAddress dns(192,168,0,1);

WiFiClient net;
AsyncWebServer server(80);

// ★ NVS 핸들
static Preferences preferences;

// ---------------- 공용 JSON 빌더 ----------------
static String buildStateJson() {
  String s = "{";
  s += "\"isRunning\":";     s += (isRunning ? "true" : "false");     s += ",";
  s += "\"emergencyStop\":"; s += (emergencyStop ? "true" : "false"); s += ",";
  s += "\"isHot\":";         s += (isHot ? "true" : "false");         s += ",";
  s += "\"isCold\":";        s += (isCold ? "true" : "false");
  s += "}";
  return s;
}

static String buildMeasurementJson() {
  DynamicJsonDocument doc(256);
  doc["Heater_1_NTC_TEMP"]   = Heater_1_NTC_TEMP;
  doc["Heater_2_NTC_TEMP"]   = Heater_2_NTC_TEMP;
  doc["YF_S402B_outputFlow"] = YF_S402B_outputFlow;
  doc["YF_S402B_inputFlow"]  = YF_S402B_inputFlow;
  doc["currentAmpere"]       = currentAmpere;
  String s; serializeJson(doc, s); return s;
}

static String buildSettingsJson() {
  DynamicJsonDocument doc(512);
  doc["create_espresso_ml_setting"]            = create_espresso_ml_setting;
  doc["create_water_ml_setting"]               = create_water_ml_setting;
  doc["create_americano_espresso_ml_setting"]  = create_americano_espresso_ml_setting;
  doc["create_americano_water_ml_setting"]     = create_americano_water_ml_setting;
  doc["create_americano_temperature_setting"]  = create_americano_temperature_setting;
  doc["create_cafelatte_espresso_ml_setting"]  = create_cafelatte_espresso_ml_setting;
  doc["create_cafelatte_milk_ml_setting"]      = create_cafelatte_milk_ml_setting;
  doc["create_cafelatte_temperature_setting"]  = create_cafelatte_temperature_setting;
  doc["cleaning_time_setting"]                 = cleaning_time_setting;
  doc["cleaning_time_all_setting"]             = cleaning_time_all_setting;
  doc["air_inhale_start_after_waiting_time_setting"] = air_inhale_start_after_waiting_time_setting;
  doc["air_inhale_time_setting"]               = air_inhale_time_setting;
  doc["air_inhale_on_time_setting"]            = air_inhale_on_time_setting;
  doc["air_inhale_off_time_setting"]           = air_inhale_off_time_setting;
  doc["create_shake_time_setting"]             = create_shake_time_setting;
  doc["GearPump_PWM_output_percent"]           = GearPump_PWM_output_percent;
  doc["drain_time_setting"]                    = drain_time_setting;
  doc["create_heater_2_off_flow_limit_percent"]= create_heater_2_off_flow_limit_percent;
  doc["emergencyAmpere"]                       = emergencyAmpere;
  doc["Heter_PID_P"]                           = Heter_PID_P;
  doc["Heter_PID_I"]                           = Heter_PID_I;
  doc["Heter_PID_D"]                           = Heter_PID_D;
  String s; serializeJson(doc, s); return s;
}

// ---------------- NVS 저장/로드 ----------------
void loadSettings() {
  preferences.begin("coffee", false);
  #define GETI(name) name = preferences.getInt(#name, name)
  #define GETF(name) name = preferences.getFloat(#name, name)
  #define GETD(name) name = preferences.getDouble(#name, name)

  GETI(create_espresso_ml_setting);
  GETI(create_water_ml_setting);
  GETI(create_americano_espresso_ml_setting);
  GETI(create_americano_water_ml_setting);
  GETI(create_americano_temperature_setting);
  GETI(create_cafelatte_espresso_ml_setting);
  GETI(create_cafelatte_milk_ml_setting);
  GETI(create_cafelatte_temperature_setting);
  GETI(cleaning_time_setting);
  GETI(cleaning_time_all_setting);
  GETI(air_inhale_start_after_waiting_time_setting);
  GETI(air_inhale_time_setting);
  GETI(air_inhale_on_time_setting);
  GETI(air_inhale_off_time_setting);
  GETI(create_shake_time_setting);
  GETI(GearPump_PWM_output_percent);
  GETI(drain_time_setting);
  GETI(create_heater_2_off_flow_limit_percent);
  GETF(emergencyAmpere);
  GETD(Heter_PID_P);
  GETD(Heter_PID_I);
  GETD(Heter_PID_D);

  preferences.end();
  Serial.println("[NVS] settings loaded");
}

void saveSetting(const String& key, int value)   { preferences.begin("coffee", false); preferences.putInt  (key.c_str(), value); preferences.end(); }
void saveSetting(const String& key, float value) { preferences.begin("coffee", false); preferences.putFloat(key.c_str(), value); preferences.end(); }
void saveSetting(const String& key, double value){ preferences.begin("coffee", false); preferences.putDouble(key.c_str(), value);preferences.end(); }

// 파라미터 이름으로 전역 변수 갱신 + NVS 저장
static void updateSettingByName(const String& name, const String& v) {
  int iv = v.toInt();
  double dv = v.toDouble();

  // 정수형
  if (name == "create_espresso_ml_setting") { create_espresso_ml_setting = iv; saveSetting(name, iv); }
  else if (name == "create_water_ml_setting"){ create_water_ml_setting = iv; saveSetting(name, iv); }
  else if (name == "create_americano_espresso_ml_setting") { create_americano_espresso_ml_setting = iv; saveSetting(name, iv); }
  else if (name == "create_americano_water_ml_setting")   { create_americano_water_ml_setting   = iv; saveSetting(name, iv); }
  else if (name == "create_americano_temperature_setting"){ create_americano_temperature_setting= iv; saveSetting(name, iv); }
  else if (name == "create_cafelatte_espresso_ml_setting"){ create_cafelatte_espresso_ml_setting= iv; saveSetting(name, iv); }
  else if (name == "create_cafelatte_milk_ml_setting")    { create_cafelatte_milk_ml_setting    = iv; saveSetting(name, iv); }
  else if (name == "create_cafelatte_temperature_setting"){ create_cafelatte_temperature_setting= iv; saveSetting(name, iv); }
  else if (name == "cleaning_time_setting")               { cleaning_time_setting               = iv; saveSetting(name, iv); }
  else if (name == "cleaning_time_all_setting")           { cleaning_time_all_setting           = iv; saveSetting(name, iv); }
  else if (name == "air_inhale_start_after_waiting_time_setting") { air_inhale_start_after_waiting_time_setting = iv; saveSetting(name, iv); }
  else if (name == "air_inhale_time_setting")             { air_inhale_time_setting             = iv; saveSetting(name, iv); }
  else if (name == "air_inhale_on_time_setting")          { air_inhale_on_time_setting          = iv; saveSetting(name, iv); }
  else if (name == "air_inhale_off_time_setting")         { air_inhale_off_time_setting         = iv; saveSetting(name, iv); }
  else if (name == "create_shake_time_setting")           { create_shake_time_setting           = iv; saveSetting(name, iv); }
  else if (name == "GearPump_PWM_output_percent")         { GearPump_PWM_output_percent         = iv; saveSetting(name, iv); }
  else if (name == "drain_time_setting")                  { drain_time_setting                  = iv; saveSetting(name, iv); }
  else if (name == "create_heater_2_off_flow_limit_percent") { create_heater_2_off_flow_limit_percent = iv; saveSetting(name, iv); }

  // 실수/더블
  else if (name == "emergencyAmpere") { emergencyAmpere = v.toFloat(); saveSetting(name, emergencyAmpere); }
  else if (name == "Heter_PID_P")     { Heter_PID_P = dv; saveSetting(name, Heter_PID_P); }
  else if (name == "Heter_PID_I")     { Heter_PID_I = dv; saveSetting(name, Heter_PID_I); }
  else if (name == "Heter_PID_D")     { Heter_PID_D = dv; saveSetting(name, Heter_PID_D); }

  Serial.printf("[UPDATE] %s = %s\n", name.c_str(), v.c_str());
}

// ---------------- WiFi ----------------
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

// ---------------- WebServer ----------------
void startWebServer() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }
  Serial.println("LittleFS mounted");
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // 상태
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildStateJson());
  });

  // 토글 / 비상 / 온도모드
  server.on("/api/isRunning/toggle", HTTP_POST, [](AsyncWebServerRequest* req) {
    isRunning = !isRunning; req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/emergencyStop", HTTP_POST, [](AsyncWebServerRequest* req) {
    emergencyStop = true; Serial.println("EmergencyStop"); req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/hot", HTTP_POST, [](AsyncWebServerRequest* req) {
    isHot = true; isCold = false; req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/cold", HTTP_POST, [](AsyncWebServerRequest* req) {
    isCold = true; isHot = false; req->send(200, "application/json", buildStateJson());
  });

  // ★ 측정(0.5초 폴링)
  server.on("/api/measurement", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildMeasurementJson());
  });

  // ★ 설정(3초 폴링)
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildSettingsJson());
  });

  // ★ 설정 즉시 업데이트(+NVS 저장)
  server.on("/api/updateSetting", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (req->hasParam("name", true) && req->hasParam("value", true)) {
      String name  = req->getParam("name", true)->value();
      String value = req->getParam("value", true)->value();
      updateSettingByName(name, value);
      req->send(200, "application/json", "{\"ok\":true}");
    } else {
      req->send(400, "text/plain", "Missing name or value");
    }
  });

  // 시작 버튼들 (시리얼 로그로 함수명 출력)
  server.on("/api/start/espresso", HTTP_POST, [](AsyncWebServerRequest* req) {
    create_espresso_ml = create_espresso_ml_setting;
    create_water_ml    = create_water_ml_setting;
    isRunning = true; isWorking = true;
    Serial.println("Start_Espresso");
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/start/americano", HTTP_POST, [](AsyncWebServerRequest* req) {
    create_espresso_ml = create_americano_espresso_ml_setting;
    create_water_ml    = create_americano_water_ml_setting;
    create_temperature = create_americano_temperature_setting;
    isRunning = true; isWorking = true;
    Serial.println("Start_Americano");
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/start/cafelatte", HTTP_POST, [](AsyncWebServerRequest* req) {
    create_espresso_ml = create_cafelatte_espresso_ml_setting;
    create_milk_ml     = create_cafelatte_milk_ml_setting;
    create_temperature = create_cafelatte_temperature_setting;
    isRunning = true; isWorking = true;
    Serial.println("Start_CafeLatte");
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/start/cleaning", HTTP_POST, [](AsyncWebServerRequest* req) {
    cleaning_time = cleaning_time_setting;
    cleaning_time_all = cleaning_time_all_setting;
    isRunning = true; isWorking = true;
    Serial.println("Start_Cleaning");
    req->send(200, "application/json", buildStateJson());
  });

  server.begin();
  Serial.println("HTTP server started");
}
