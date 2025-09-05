// wifiSetup.cpp
#include "wifiSetup.h"
#include "variables.h"
#include "machineRunning.h"
#include "MCP23017.h"

// FreeRTOS 헤더 추가
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 와이파이 정보
const char* wifi_ssid = "NNX2-2.4G";
const char* wifi_password = "$@43skshslrtm";
const IPAddress wifiIP(192, 168, 0, 231);
const IPAddress gateway(192, 168, 0, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns(192, 168, 0, 1);

WiFiClient net;
AsyncWebServer server(80);

static Preferences preferences;

// 상태 JSON
static String buildStateJson() {
  String s = "{";
  s += "\"isRunning\":";
  s += (isRunning ? "true" : "false");
  s += ",";
  s += "\"isWorking\":";
  s += (isWorking ? "true" : "false");
  s += ",";  // ★ 추가
  s += "\"emergencyStop\":";
  s += (emergencyStop ? "true" : "false");
  s += ",";
  s += "\"isHot\":";
  s += (isHot ? "true" : "false");
  s += ",";
  s += "\"isCold\":";
  s += (isCold ? "true" : "false");
  s += "}";
  return s;
}

// GPIO 상태 갱신 함수
static void readMachineGPIO() {
  ESP32_GPIO25 = digitalRead(Heater_1_GPIO_PIN);
  expanderGPIO1 = expanderReadForDoc(1);
  expanderGPIO2 = expanderReadForDoc(2);
  expanderGPIO3 = expanderReadForDoc(3);
  expanderGPIO4 = expanderReadForDoc(4);
  expanderGPIO5 = expanderReadForDoc(5);
  expanderGPIO6 = expanderReadForDoc(6);
  expanderGPIO7 = expanderReadForDoc(7);
  expanderGPIO8 = expanderReadForDoc(8);
  expanderGPIO9 = expanderReadForDoc(9);
  expanderGPIO10 = expanderReadForDoc(10);
}

// 측정값 JSON
static String buildMeasurementJson() {
  DynamicJsonDocument doc(512);  // 크기 조금 늘림
  doc["Heater_1_NTC_TEMP"] = Heater_1_NTC_TEMP;
  doc["Heater_2_NTC_TEMP"] = Heater_2_NTC_TEMP;
  doc["YF_S402B_outputFlow"] = YF_S402B_outputFlow;
  doc["YF_S402B_inputFlow"] = YF_S402B_inputFlow;
  doc["currentAmpere"] = currentAmpere;
  doc["Heater_2_PWM_output_value"] = Heater_2_PWM_output_value;  // ★ 추가

  // ★ GPIO 상태값 추가
  doc["ESP32_GPIO25"] = ESP32_GPIO25 ? 1 : 0;
  doc["GPIO1"] = expanderGPIO1 ? 1 : 0;
  doc["GPIO2"] = expanderGPIO2 ? 1 : 0;
  doc["GPIO3"] = expanderGPIO3 ? 1 : 0;
  doc["GPIO4"] = expanderGPIO4 ? 1 : 0;
  doc["GPIO5"] = expanderGPIO5 ? 1 : 0;
  doc["GPIO6"] = expanderGPIO6 ? 1 : 0;
  doc["GPIO7"] = expanderGPIO7 ? 1 : 0;
  doc["GPIO8"] = expanderGPIO8 ? 1 : 0;
  doc["GPIO9"] = expanderGPIO9 ? 1 : 0;
  doc["GPIO10"] = expanderGPIO10 ? 1 : 0;
   // CT 아날로그 값 전송
   doc["ctAnalogValue"] = ctAnalogValue;
  String s;
  serializeJson(doc, s);
  return s;
}
// 설정값 JSON (변수명과 동일한 키, 15자 이하)
static String buildSettingsJson() {
  DynamicJsonDocument doc(512);
  doc["e_ml_set"] = e_ml_set;
  doc["e_tmp_set"] = e_tmp_set;
  doc["a_e_ml_set"] = a_e_ml_set;
  doc["a_w_ml_set"] = a_w_ml_set;
  doc["a_tmp_set"] = a_tmp_set;
  doc["c_e_ml_set"] = c_e_ml_set;
  doc["c_m_ml_set"] = c_m_ml_set;
  doc["c_tmp_set"] = c_tmp_set;
  doc["clean_time"] = clean_time;
  doc["clean_time_all"] = clean_time_all;
  doc["inhale_w_time"] = inhale_w_time;
  doc["inhale_time"] = inhale_time;
  doc["inhale_on_time"] = inhale_on_time;
  doc["inhale_off_time"] = inhale_off_time;
  doc["shake_time"] = shake_time;
  doc["pump_out_per"] = pump_out_per;
  doc["drain_time"] = drain_time;
  doc["h2_limit_per"] = h2_limit_per;
  doc["emergencyA"] = emergencyA;
  doc["Heter_PID_P"] = Heter_PID_P;
  doc["Heter_PID_I"] = Heter_PID_I;
  doc["Heter_PID_D"] = Heter_PID_D;
  doc["ctAdcZero"] = ctAdcZero;
  String s;
  serializeJson(doc, s);
  return s;
}

// 설정값 로드
void loadSettings() {
  preferences.begin("coffee", false);
  e_ml_set = preferences.getInt("e_ml_set", e_ml_set);
  e_tmp_set = preferences.getInt("e_tmp_set", e_tmp_set);
  a_e_ml_set = preferences.getInt("a_e_ml_set", a_e_ml_set);
  a_w_ml_set = preferences.getInt("a_w_ml_set", a_w_ml_set);
  a_tmp_set = preferences.getInt("a_tmp_set", a_tmp_set);
  c_e_ml_set = preferences.getInt("c_e_ml_set", c_e_ml_set);
  c_m_ml_set = preferences.getInt("c_m_ml_set", c_m_ml_set);
  c_tmp_set = preferences.getInt("c_tmp_set", c_tmp_set);
  clean_time = preferences.getInt("clean_time", clean_time);
  clean_time_all = preferences.getInt("clean_time_all", clean_time_all);
  inhale_w_time = preferences.getInt("inhale_w_time", inhale_w_time);
  inhale_time = preferences.getInt("inhale_time", inhale_time);
  inhale_on_time = preferences.getInt("inhale_on_time", inhale_on_time);
  inhale_off_time = preferences.getInt("inhale_off_time", inhale_off_time);
  shake_time = preferences.getInt("shake_time", shake_time);
  pump_out_per = preferences.getInt("pump_out_per", pump_out_per);
  drain_time = preferences.getInt("drain_time", drain_time);
  h2_limit_per = preferences.getInt("h2_limit_per", h2_limit_per);
  emergencyA = preferences.getFloat("emergencyA", emergencyA);
  Heter_PID_P = preferences.getDouble("Heter_PID_P", Heter_PID_P);
  Heter_PID_I = preferences.getDouble("Heter_PID_I", Heter_PID_I);
  Heter_PID_D = preferences.getDouble("Heter_PID_D", Heter_PID_D);
  ctAdcZero = preferences.getInt("ctAdcZero", ctAdcZero);
  preferences.end();
  Serial.println("[NVS] settings loaded");
}

// NVS 저장
void saveSetting(const String& key, int value) {
  preferences.begin("coffee", false);
  preferences.putInt(key.c_str(), value);
  preferences.end();
}
void saveSetting(const String& key, float value) {
  preferences.begin("coffee", false);
  preferences.putFloat(key.c_str(), value);
  preferences.end();
}
void saveSetting(const String& key, double value) {
  preferences.begin("coffee", false);
  preferences.putDouble(key.c_str(), value);
  preferences.end();
}

// 설정 업데이트 (이름과 값으로 구분)
static void updateSettingByName(const String& name, const String& v) {
  int iv = v.toInt();
  double dv = v.toDouble();
  if (name == "e_ml_set") {
    e_ml_set = iv;
    saveSetting(name, iv);
  } else if (name == "e_tmp_set") {
    e_tmp_set = iv;
    saveSetting(name, iv);
  } else if (name == "a_e_ml_set") {
    a_e_ml_set = iv;
    saveSetting(name, iv);
  } else if (name == "a_w_ml_set") {
    a_w_ml_set = iv;
    saveSetting(name, iv);
  } else if (name == "a_tmp_set") {
    a_tmp_set = iv;
    saveSetting(name, iv);
  } else if (name == "c_e_ml_set") {
    c_e_ml_set = iv;
    saveSetting(name, iv);
  } else if (name == "c_m_ml_set") {
    c_m_ml_set = iv;
    saveSetting(name, iv);
  } else if (name == "c_tmp_set") {
    c_tmp_set = iv;
    saveSetting(name, iv);
  } else if (name == "clean_time") {
    clean_time = iv;
    saveSetting(name, iv);
  } else if (name == "clean_time_all") {
    clean_time_all = iv;
    saveSetting(name, iv);
  } else if (name == "inhale_w_time") {
    inhale_w_time = iv;
    saveSetting(name, iv);
  } else if (name == "inhale_time") {
    inhale_time = iv;
    saveSetting(name, iv);
  } else if (name == "inhale_on_time") {
    inhale_on_time = iv;
    saveSetting(name, iv);
  } else if (name == "inhale_off_time") {
    inhale_off_time = iv;
    saveSetting(name, iv);
  } else if (name == "shake_time") {
    shake_time = iv;
    saveSetting(name, iv);
  } else if (name == "pump_out_per") {
    pump_out_per = iv;
    saveSetting(name, iv);
  } else if (name == "drain_time") {
    drain_time = iv;
    saveSetting(name, iv);
  } else if (name == "h2_limit_per") {
    h2_limit_per = iv;
    saveSetting(name, iv);
  } else if (name == "emergencyA") {
    emergencyA = v.toFloat();
    saveSetting(name, emergencyA);
  } else if (name == "Heter_PID_P") {
    Heter_PID_P = dv;
    saveSetting(name, Heter_PID_P);
  } else if (name == "Heter_PID_I") {
    Heter_PID_I = dv;
    saveSetting(name, Heter_PID_I);
  } else if (name == "Heter_PID_D") {
    Heter_PID_D = dv;
    saveSetting(name, Heter_PID_D);
  } else if (name == "Heater_2_PWM_output_value") {
    Heater_2_PWM_output_value = iv;
    saveSetting(name, iv);
  } else if (name == "ctAdcZero") { 
    ctAdcZero = iv; 
    saveSetting(name, iv); 
  }
  Serial.printf("[UPDATE] %s = %s\n", name.c_str(), v.c_str());
}

// WiFi 연결
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

// Web 서버 시작
void startWebServer() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }
  Serial.println("LittleFS mounted");

  // 정적 파일 제공
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // 상태
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildStateJson());
  });

  // 토글/비상/온도 모드
  server.on("/api/isRunning/toggle", HTTP_POST, [](AsyncWebServerRequest* req) {
    isRunning = !isRunning;
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/emergencyStop", HTTP_POST, [](AsyncWebServerRequest* req) {
    emergencyStop = true;
    isRunning = false;
    isWorking = false;
    Serial.println("EmergencyStop");
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/hot", HTTP_POST, [](AsyncWebServerRequest* req) {
    isHot = true;
    isCold = false;
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/cold", HTTP_POST, [](AsyncWebServerRequest* req) {
    isCold = true;
    isHot = false;
    req->send(200, "application/json", buildStateJson());
  });

  // 측정값 (0.5초마다)
  server.on("/api/measurement", HTTP_GET, [](AsyncWebServerRequest* req) {
    readMachineGPIO();  // ★ 측정 전 GPIO 상태 갱신
    req->send(200, "application/json", buildMeasurementJson());
  });

  // 설정값 (3초마다)
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildSettingsJson());
  });

  // 설정 업데이트
  server.on("/api/updateSetting", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (req->hasParam("name", true) && req->hasParam("value", true)) {
      String name = req->getParam("name", true)->value();
      String value = req->getParam("value", true)->value();
      updateSettingByName(name, value);
      req->send(200, "application/json", "{\"ok\":true}");
    } else {
      req->send(400, "text/plain", "Missing name or value");
    }
  });

  // 제조/청소 시작 (에스프레소)
  server.on("/api/start/espresso", HTTP_POST, [](AsyncWebServerRequest* req) {
    // 에스프레소 추출에 필요한 값 설정
    c_esspresso_ml = 0;
    c_water_ml = 0;
    c_milk_ml = 0;
    c_tmp = 0;
    // 사용자 설정값으로 대입 (에스프레소는 우유/물 사용 안 함)
    c_esspresso_ml = e_ml_set;
    c_tmp = e_tmp_set;
    // 시스템 실행 상태 설정. isWorking은 createEspresso() 내부에서 처리
    Serial.println("Start_Espresso");
    // 즉시 응답 전송
    req->send(200, "application/json", buildStateJson());
    // 비동기적으로 추출 태스크 생성 (핸들 사용 안 함)
    xTaskCreatePinnedToCore(espressoTask, "EspressoTask", 4096, NULL, 1, NULL, 1);
  });
  server.on("/api/start/americano", HTTP_POST, [](AsyncWebServerRequest* req) {
    // 아메리카노 추출에 필요한 값 설정
    c_esspresso_ml = a_e_ml_set;
    c_water_ml = a_w_ml_set;
    c_tmp = a_tmp_set;
    Serial.println("Start_Americano");
    req->send(200, "application/json", buildStateJson());
    // 비동기적으로 아메리카노 추출 태스크 생성
  });
  server.on("/api/start/cafelatte", HTTP_POST, [](AsyncWebServerRequest* req) {
    c_esspresso_ml = c_e_ml_set;
    c_milk_ml = c_m_ml_set;
    c_tmp = c_tmp_set;
    isRunning = true;
    isWorking = true;
    Serial.println("Start_CafeLatte");
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/start/cleaning", HTTP_POST, [](AsyncWebServerRequest* req) {
    // clean_time과 clean_time_all은 필요에 따라 c_* 변수에 대입하지 않아도 됩니다.
    isRunning = true;
    isWorking = true;
    Serial.println("Start_Cleaning");
    req->send(200, "application/json", buildStateJson());
  });

  server.begin();
  Serial.println("HTTP server started");
}
