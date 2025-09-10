// wifiSetup.cpp
#include "wifiSetup.h"
#include "variables.h"
#include "machineRunning.h"
#include "MCP23017.h"

// PWM 제어 함수와 상태 플래그를 사용하기 위해 추가 헤더 포함
#include "GearPump_PWM.h"
#include "Heater.h"

// FreeRTOS 헤더 추가
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 와이파이 정보
// const char* wifi_ssid = "NNX2-2.4G";
const char* wifi_ssid = "nnx-factory 3 2.4G";
const char* wifi_password = "$@43skshslrtm";
const IPAddress wifiIP(192, 168, 40, 27);  // 접속할 고정 IP 설정(이미 사용중일 경우 동작 에러날 수 있음)
const IPAddress gateway(192, 168, 40, 1);
// const IPAddress wifiIP(192, 168, 0, 231); // 접속할 고정 IP 설정(이미 사용중일 경우 동작 에러날 수 있음)
// const IPAddress gateway(192, 168, 0, 1);
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
  ESP32_GPIO25 = digitalRead(Heater2_GPIO_RELAY_PIN);
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
  doc["Heater1_output_value"] = Heater1_output_value;  // ★ 추가

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

  // === PWM 상태값 추가 ===
  // 기어펌프 PWM(ESP32 GPIO32)와 히터2 PWM(ESP32 GPIO33)의 상태를 전송합니다.
  // gearPumpOn과 heater2_On는 각각 GearPump_PWM.cpp와 Heater.cpp에서 PWM을 켜거나
  // 끌 때 갱신되는 상태 플래그입니다. 출력값이 계산되어도 실제로 PWM을
  // 출력하지 않는 경우를 구분하기 위해 플래그를 사용합니다.
  doc["PWM32"] = gearPumpOn ? 1 : 0;
  doc["PWM33"] = heater2_On ? 1 : 0;
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
  
  doc["c_tmp"] = c_tmp;
  // PID 제어 윈도우 주기(초)
  // 히터 위험 온도 설정값 추가
  // 프론트엔드에서 h1_emer_tmp (히터1), h2_emer_tmp2 (히터2) 값을 읽어 수정할 수 있습니다.
  doc["h1_emer_tmp"] = h1_emer_tmp;
  doc["h2_emer_tmp2"] = h2_emer_tmp2;
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

  // PID 제어용 현재 목표 온도 값을 로드합니다.
  c_tmp = preferences.getInt("c_tmp", c_tmp);
  // --- PID 윈도우 주기(sec, ms) 로드 + 일관화 ---
  // 윈도우 주기 로드
  preferences.end();
  Serial.println("[NVS] settings loaded");
  

  // 히터 위험 온도 설정값 로드 (float)
  h1_emer_tmp = preferences.getFloat("h1_emer_tmp", h1_emer_tmp);
  h2_emer_tmp2 = preferences.getFloat("h2_emer_tmp2", h2_emer_tmp2);
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
  }else if (name == "ctAdcZero") {
    ctAdcZero = iv;
    saveSetting(name, iv);
  } else if (name == "h1_emer_tmp") {
    // 히터1 위험 온도 설정값(float)
    h1_emer_tmp = v.toFloat();
    saveSetting(name, h1_emer_tmp);
  } else if (name == "h2_emer_tmp2") {
    // 히터2 위험 온도 설정값(float)
    h2_emer_tmp2 = v.toFloat();
    saveSetting(name, h2_emer_tmp2);
  } else if (name == "c_tmp") {
    // PID 제어용 목표 온도값 설정 (정수)
    c_tmp = iv;
    saveSetting(name, iv);
    Heater1_OnSetpointChanged();
  }
  Serial.printf("[UPDATE] %s = %s\n", name.c_str(), v.c_str());
}

// WiFi 연결
void wifiSetup() {
  if (WiFi.status() != WL_CONNECTED) {

    // 미사용시 주석처리
    // 고정 IP 설정
    if (!WiFi.config(wifiIP, gateway, subnet, dns)) {
      Serial.println("STA Failed to configure");
    }
    // 고정IP 설정 끝
    /*
      WIFI_STA (Station 모드): 현재 코드처럼 다른 와이파이 네트워크에 **'연결'**할 때 사용합니다. 로봇과 통신하려면 로봇이 연결된 공유기에 접속해야 하므로 이 모드를 사용해야 합니다.
      WIFI_AP (Access Point 모드): ESP32가 직접 와이파이 신호를 뿌리는 '공유기' 역할을 합니다. 스마트폰 등으로 ESP32에 직접 접속해야 할 때 사용합니다.
      WIFI_AP_STA (AP + Station 동시 모드): 위 두 가지 모드를 동시에 사용합니다.
    */
    WiFi.mode(WIFI_STA);
    /*
      전력 절감 모드를 비활성화하여 응답 지연(latency)을 최소화합니다.
      실시간 통신에서 가장 효과적인 최적화 중 하나입니다.
    */
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);  // 자동 재연결
    WiFi.persistent(false);       // 설정을 Flash에 저장하지 않음
    /*
      설명 : WIFI_POWER_19_5dBm 모드는 최대 전력을 사용하는 모드입니다.
      즉, 최대 전력을 사용하여 와이파이 신호를 뿌립니다.
      이 모드는 최대 전력을 사용하여 와이파이 신호를 뿌립니다.
    */
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
  delay(500);
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
    emergencyStop = false;
    Serial.println("System On Button Touched");
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/emergencyStop", HTTP_POST, [](AsyncWebServerRequest* req) {
    emergencyStop = true;
    {
      isRunning = false;
      isWorking = false;
      // 모든 출력을 끕니다. 비상 상황에서 호출되므로 현재 진행 중인 작업을 완전히 정지합니다.
      // 기어펌프 및 히터 OFF
      GearPump_PWM_OFF();
      if (isHot) {
        Heater1_GPIO_OFF();
        Heater2_GPIO_OFF();
      }
      // 모든 expander 핀을 LOW로 설정하여 솔레노이드 밸브/펌프/밸브를 종료합니다.
      for (int pin = 1; pin <= 10; ++pin) {
        expanderWriteForDoc(pin, LOW);
      }
      // 시스템 상태 플래그 리셋
      if (isHot) {
        Heater1_GPIO_OFF();
        Heater2_GPIO_OFF();
      }
      emergencyStop = false;
      c_esspresso_ml = 0;
      c_water_ml = 0;
      c_milk_ml = 0;
      c_tmp = 0;
      YF_S402B_outputFlow = 0;
      YF_S402B_inputFlow = 0;
      
    }
    Serial.println("EmergencyStop Button Touched");
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/hot", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (!isWorking) {
      isHot = true;
      isCold = false;
    }
    Serial.println("Hot Button Touched");
    req->send(200, "application/json", buildStateJson());
  });
  server.on("/api/cold", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (!isWorking) {
      isCold = true;
      isHot = false;
    }
    isHot = false;
    Serial.println("Cold Button Touched");
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
    Serial.println("Start_Espresso");
    req->send(200, "application/json", buildStateJson());
    xTaskCreatePinnedToCore(espressoTask, "EspressoTask", 4096, NULL, 1, NULL, 1);
  });
  server.on("/api/start/americano", HTTP_POST, [](AsyncWebServerRequest* req) {
    Serial.println("Start_Americano");
    req->send(200, "application/json", buildStateJson());
    xTaskCreatePinnedToCore(americanoTask, "AmericanoTask", 4096, NULL, 1, NULL, 1);
  });
  server.on("/api/start/cafelatte", HTTP_POST, [](AsyncWebServerRequest* req) {
    Serial.println("Start_CafeLatte");
    req->send(200, "application/json", buildStateJson());
    xTaskCreatePinnedToCore(cafelatteTask, "CafelatteTask", 4096, NULL, 1, NULL, 1);
  });
  server.on("/api/start/cleaning", HTTP_POST, [](AsyncWebServerRequest* req) {
    Serial.println("Start_Cleaning");
    req->send(200, "application/json", buildStateJson());
    xTaskCreatePinnedToCore(cleaningTask, "CleaningTask", 4096, NULL, 1, NULL, 1);
  });


  // 사용자 정의 GPIO/PWM 토글 엔드포인트를 정의합니다.
  // 이 엔드포인트는 server.begin() 이전에 등록해야 합니다.
  server.on("/api/gpio/toggle", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (req->hasParam("name", true)) {
      String name = req->getParam("name", true)->value();
      // ESP32 GPIO25 제어 (히터1 릴레이)
      if (name == "ESP32_GPIO25") {
        // 수동 히터1 상태를 토글합니다.
        heater1_On = !heater1_On;

        if (heater1_On) {
          Serial.println("Heater1_GPIO_On");
          Heater1_GPIO_ON();

          ESP32_GPIO25 = true;
        } else {
          Serial.println("Heater1_GPIO_Off");
          Heater1_GPIO_OFF();

        }
      } else if (name.startsWith("GPIO")) {
        // 확장자 핀. 이름에서 숫자 추출 (GPIO1 ~ GPIO10)
        int pinNum = name.substring(4).toInt();
        if (pinNum >= 1 && pinNum <= 10) {
          bool current = expanderReadForDoc(pinNum);
          bool newVal = !current;
          expanderWriteForDoc(pinNum, newVal ? HIGH : LOW);
          // 관련 전역 상태 변수 갱신
          switch (pinNum) {
            case 1: expanderGPIO1 = newVal; break;
            case 2: expanderGPIO2 = newVal; break;
            case 3: expanderGPIO3 = newVal; break;
            case 4: expanderGPIO4 = newVal; break;
            case 5: expanderGPIO5 = newVal; break;
            case 6: expanderGPIO6 = newVal; break;
            case 7: expanderGPIO7 = newVal; break;
            case 8: expanderGPIO8 = newVal; break;
            case 9: expanderGPIO9 = newVal; break;
            case 10: expanderGPIO10 = newVal; break;
          }
        }
      } else if (name == "PWM32") {
        // 기어펌프 PWM 토글
        gearPumpOn = !gearPumpOn;
        if (gearPumpOn) {
          GearPump_PWM_ON();
        } else {
          GearPump_PWM_OFF();
        }
      } else if (name == "PWM33") {
        // 히터2 PWM 토글
        heater2_On = !heater2_On;
        if (heater2_On) {
          Serial.println("Heater2_GPIO_ON");
          Heater2_GPIO_ON();
          // digitalWrite(Heater1_GPIO_SSR_PIN, HIGH);
        } else {
          Serial.println("Heater2_GPIO_OFF");
          Heater2_GPIO_OFF();
          // digitalWrite(Heater1_GPIO_SSR_PIN, LOW);
        }
      }
      // 토글 후 측정 JSON을 반환하여 프론트엔드가 갱신하도록 합니다.
      String json = buildMeasurementJson();
      req->send(200, "application/json", json);
    } else {
      req->send(400, "text/plain", "Missing name parameter");
    }
  });

  server.begin();
  Serial.println("HTTP server started");
}
