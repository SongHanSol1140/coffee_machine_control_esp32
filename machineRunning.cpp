// machineRunning.cpp
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "variables.h"
#include "machineRunning.h"

// GPIO 확장자와 기어펌프 제어 함수 사용을 위해 헤더 포함
#include "MCP23017.h"
#include "GearPump_PWM.h"
// 히터2 PWM 상태 갱신을 위해 Heater2_GPIO_ON() 함수를 사용합니다.
#include "Heater.h"

// ===================================================================
// ===================================================================
// 공기흡입 병행작동을 위한 테스크
// ===== 공기흡입 병렬 태스크 지원 =====
typedef struct {
  uint32_t waitMs;    // 시작 대기
  uint32_t totalMs;   // 전체 흡입 시간
  uint32_t onMs;      // ON 구간
  uint32_t offMs;     // OFF 구간
  uint8_t  valvePin;  // 공기흡입 밸브 (기존 7번)
} InhaleArgs;

static TaskHandle_t sInhaleTask = NULL;
static volatile bool sInhaleStopReq = false;
static volatile bool sInhaleRunning = false;

static void inhaleTask(void* pv) {
  // 인자 복사 후 힙 해제
  InhaleArgs args = *(InhaleArgs*)pv;
  free(pv);

  sInhaleRunning = true;

  // 1) 시작 대기
  uint32_t t0 = millis();
  while (!sInhaleStopReq && (millis() - t0) < args.waitMs) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  // 2) ON/OFF 반복 (총 args.totalMs)
  uint32_t cycleStart = millis();
  while (!sInhaleStopReq && (millis() - cycleStart) < args.totalMs) {
    // 남은 시간 계산
    uint32_t elapsed   = millis() - cycleStart;
    uint32_t remaining = (elapsed >= args.totalMs) ? 0 : (args.totalMs - elapsed);
    if (remaining == 0) break;

    // ON
    uint32_t curOnMs = (args.onMs > remaining) ? remaining : args.onMs;
    expanderWriteForDoc(args.valvePin, HIGH);
    uint32_t onStart = millis();
    while (!sInhaleStopReq && (millis() - onStart) < curOnMs) {
      vTaskDelay(pdMS_TO_TICKS(5));
    }
    expanderWriteForDoc(args.valvePin, LOW);
    if (sInhaleStopReq) break;

    // 남은 시간 갱신
    elapsed   = millis() - cycleStart;
    remaining = (elapsed >= args.totalMs) ? 0 : (args.totalMs - elapsed);
    if (remaining == 0) break;

    // OFF
    uint32_t curOffMs = (args.offMs > remaining) ? remaining : args.offMs;
    uint32_t offStart = millis();
    while (!sInhaleStopReq && (millis() - offStart) < curOffMs) {
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }

  // 3) 종료 시 OFF 보장
  expanderWriteForDoc(args.valvePin, LOW);

  sInhaleRunning = false;
  sInhaleTask = NULL;
  vTaskDelete(NULL);
}

// 이미 떠있는 흡입 태스크를 안전하게 중지
static void cancelInhaleTask(bool wait, uint32_t timeoutMs = 1000) {
  if (sInhaleTask) {
    sInhaleStopReq = true;
    uint32_t t0 = millis();
    while (wait && sInhaleRunning && (millis() - t0) < timeoutMs) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    // 밸브 OFF 강제 보장
    expanderWriteForDoc(7, LOW);
    // 혹시 남아있으면 강제 삭제
    if (sInhaleTask) {
      vTaskDelete(sInhaleTask);
      sInhaleTask = NULL;
      sInhaleRunning = false;
    }
    sInhaleStopReq = false;
  } else {
    // 혹시 모를 잔류 ON 차단
    expanderWriteForDoc(7, LOW);
  }
}

// 흡입 태스크 시작 (기존 값들을 ms로 환산해서 넣어 호출)
static bool startInhaleTask(uint32_t waitMs, uint32_t totalMs, uint32_t onMs, uint32_t offMs,
                            uint8_t valvePin = 7, UBaseType_t priority = 1, BaseType_t core = 1) {
  // 혹시 살아있는 이전 태스크 정리
  cancelInhaleTask(true, 200);

  InhaleArgs* args = (InhaleArgs*)malloc(sizeof(InhaleArgs));
  if (!args) return false;
  args->waitMs  = waitMs;
  args->totalMs = totalMs;
  args->onMs    = onMs;
  args->offMs   = offMs;
  args->valvePin = valvePin;

  sInhaleStopReq = false;
  sInhaleRunning = false;
  BaseType_t ok = xTaskCreatePinnedToCore(
    inhaleTask, "InhaleTask", 2048, args, priority, &sInhaleTask, core
  );
  return (ok == pdPASS);
}
// ===================================================================
// ===================================================================
bool checkStart() {
  if (!isRunning) {
    Serial.println("checkStart: isRunning is false");
    return false;
  }
  if (isWorking) {
    Serial.println("checkStart: isWorking is true");
    return false;
  }
  return true;
}

void stopAllOutputsAndResetFlags() {
  // 공기흡입 태스크 중지 (대기하며 종료)
  cancelInhaleTask(true);

  
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

void createValueReset() {
  // 혹시 이전 흡입 태스크 잔존 시 정리
  cancelInhaleTask(true, 200);
  c_esspresso_ml = 0;
  c_water_ml = 0;
  c_milk_ml = 0;
  c_tmp = 0;
  YF_S402B_outputFlow = 0;
  YF_S402B_inputFlow = 0;
}
// ===================================================================
// ===================================================================
// ===================================================================
void createEspresso() {
  // 2. Start조건 확인 (isRunning, isWorking)
  if (!checkStart()) {
    Serial.println("Create Espresso: checkStart is false");
    return;
  } else {
    createValueReset();
    isWorking = true;
    c_esspresso_ml = e_ml_set;
    // 에스프레소 제조 온도 설정값이 0보다 크면 PID 목표 온도(c_tmp)를 갱신합니다.
    // 값이 0인 경우 이전에 설정된 c_tmp를 그대로 사용하여 PID 제어를 유지합니다.
    c_tmp = e_tmp_set;
    Serial.println("Create Espresso Start");
  };


  // 3. 순환 3Way Valve ON
  expanderWriteForDoc(6, HIGH);
  // 4. 먼저 기어펌프를 켭니다 (유량을 먼저 확보)
  GearPump_PWM_ON();
  // 5. 히터는 필요한 양의 약 10%가 이동한 뒤에 켭니다.
  float startVolume = (float)e_ml_set * 0.1f;
  while (YF_S402B_outputFlow <= startVolume) {
    // 비상 정지나 과전류 검사를 수행하며 짧게 대기합니다.
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 10% 유량 달성 후 히터 ON
  if (isHot) {
    Heater1_GPIO_ON();
    Heater2_GPIO_ON();

  }

  Serial.println("Tast Check 2");
  // 드레인 시간만큼 대기 (ms) 동안 비상 상황과 과열을 체크합니다.
  unsigned long startMs = millis();
  unsigned long drainMs = (unsigned long)drain_time * 1000UL;
  while (millis() - startMs < drainMs) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    // 히터가 켜진 경우 온도 과열에 따라 ON/OFF 제어
    if (isHot) {
      // Heater 1: 릴레이 제어
      if (Heater_1_NTC_TEMP >= h1_emer_tmp) {
        Heater1_GPIO_OFF();
      } else {
        Heater1_GPIO_ON();
      }
      // Heater 2: PWM 제어 (PID 출력 사용)
      if (Heater_2_NTC_TEMP >= h2_emer_tmp2) {
        Heater2_GPIO_OFF();
      } else {
        Heater2_GPIO_ON();
      }
    }
    delay(10);
  }

  // GPIO Expander #8 High (추출 완료 후 배출 시작)
  expanderWriteForDoc(5, HIGH);
  // 유량계 #1 출구 값이 limit 퍼센트까지 도달할 때까지 대기
  float limitVolume = (float)e_ml_set * ((float)h2_limit_per / 100.0f);
  while (YF_S402B_outputFlow <= limitVolume) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    if (isHot) {
      if (Heater_1_NTC_TEMP >= h1_emer_tmp) {
        Heater1_GPIO_OFF();
      } else {
        Heater1_GPIO_ON();
      }
      if (Heater_2_NTC_TEMP >= h2_emer_tmp2) {
        Heater2_GPIO_OFF();
      } else {
        Heater2_GPIO_ON();
      }
    }
    delay(10);
  }
  // 히터 1, 히터 2 출력 정지
  if (isHot) {
    // 히터1 릴레이는 바로 OFF
    Heater1_GPIO_OFF();
    // 히터2 PWM을 완전히 끄고 상태 플래그를 업데이트합니다.
    Heater2_GPIO_OFF();
  }

  // 유량계 출구가 목표값까지 도달할 때까지 대기
  while (YF_S402B_outputFlow <= (float)e_ml_set) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }

  // 기어펌프 PWM OFF
  GearPump_PWM_OFF();
  // expander #7, #8 출력 정지
  expanderWriteForDoc(6, LOW);
  expanderWriteForDoc(5, LOW);

  // 제조 완료 상태
  createValueReset();
  isWorking = false;
  Serial.println("Create Espresso Clear");
  // 초기화
};
// ===================================================================
// ===================================================================
// ===================================================================

void createAmericano() {
  // 2. Start 조건 확인 (isRunning, isWorking), 제조값 초기화
  if (!checkStart()) {
    Serial.println("Create Americano: checkStart is false");
    return;
  } else {
    isWorking = true;
    createValueReset();
    c_esspresso_ml = a_e_ml_set;
    c_water_ml = a_w_ml_set;
    c_tmp = a_tmp_set;

    Serial.println("Create Americano Start");

  }

  // 3. 에스프레소 출구 바이패스 ON
  expanderWriteForDoc(1, HIGH);
  // 4. 혼합 입구 펌프 ON
  expanderWriteForDoc(8, HIGH);

  // 5. 입력 유량계가 에스프레소 설정값 도달까지 대기
  while (YF_S402B_inputFlow <= (float)a_e_ml_set) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 6. 혼합 입구 펌프 OFF
  expanderWriteForDoc(8, LOW);
  // 7. 입력 유량계 누적값 초기화
  YF_S402B_inputFlow = 0;

  // 8. 정수 물 전자변 ON
  expanderWriteForDoc(10, HIGH);

  // 9. 물 설정값 도달까지 대기
  while (YF_S402B_inputFlow <= (float)a_w_ml_set) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }

  // 10. 정수 물 전자변 OFF
  expanderWriteForDoc(10, LOW);

  // 11. 기어펌프 ON (혼합 시작)
  GearPump_PWM_ON();

  // 12. 혼합 시간 대기 (가독성용 블록)
  {
    unsigned long startMs = millis();
    unsigned long mixMs = (unsigned long)shake_time * 1000UL;
    while (millis() - startMs < mixMs) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      delay(10);
    }
  }

  // 13. 필요한 양의 10%가 이동할 때까지 대기 후 히터 ON
  float totalVol = (float)a_e_ml_set + (float)a_w_ml_set;
  float startVolume = totalVol * 0.1f;
  while (YF_S402B_outputFlow <= startVolume) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  if (isHot) {
    // 13-14 10% 유량 달성 후 히터 ON
    Heater1_GPIO_ON();
    Heater2_GPIO_ON();
  }

  // 15. 공기 흡입 로직 (대기 → ON/OFF 반복)
  // === (변경) 공기흡입: 별도 태스크 시작 ===
  bool inhaleOk = startInhaleTask(
    (uint32_t)inhale_w_time * 1000UL,
    (uint32_t)inhale_time   * 1000UL,
    (uint32_t)inhale_on_time  * 1000UL,
    (uint32_t)inhale_off_time * 1000UL,
    7 /* valvePin */, 1 /*prio*/, 1 /*core*/
  );
  if (!inhaleOk) {
    Serial.println("[WARN] InhaleTask start failed");
  }
  // 16. 순환 3Way Valve ON
  expanderWriteForDoc(6, HIGH);

  // 17. 드레인 시간 대기
  {
    unsigned long startMs = millis();
    unsigned long drainMs = (unsigned long)drain_time * 1000UL;
    while (millis() - startMs < drainMs) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      // 히터가 켜진 경우 과열 여부에 따라 ON/OFF를 제어하고 PWM 출력을 갱신합니다.
      if (isHot) {
        // Heater 1: 릴레이 제어
        if (Heater_1_NTC_TEMP >= h1_emer_tmp) {
          Heater1_GPIO_OFF();
        } else {
          Heater1_GPIO_ON();
        }
        // Heater 2: PWM 제어 (PID 출력 사용)
        if (Heater_2_NTC_TEMP >= h2_emer_tmp2) {
          Heater2_GPIO_OFF();
        } else {
          Heater2_GPIO_ON();
        }
      }
      delay(10);
    }
  }

  // 18. 출구 3Way Valve ON
  expanderWriteForDoc(5, HIGH);

  // 19. 히터 정지 유량비율까지 대기
  {
    YF_S402B_outputFlow = 0;
    float limitVolume = (float)(a_e_ml_set + a_w_ml_set) * ((float)h2_limit_per / 100.0f);
    while (YF_S402B_outputFlow <= limitVolume) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      // 히터가 켜진 경우 과열 여부에 따라 ON/OFF를 제어하고 PWM 출력을 갱신합니다.
      if (isHot) {
        // Heater 1: 릴레이 제어
        if (Heater_1_NTC_TEMP >= h1_emer_tmp) {
          Heater1_GPIO_OFF();
        } else {
          Heater1_GPIO_ON();
        }
        // Heater 2: PWM 제어 (PID 출력 사용)
        if (Heater_2_NTC_TEMP >= h2_emer_tmp2) {
          Heater2_GPIO_OFF();
        } else {
          Heater2_GPIO_ON();
        }
      }
      delay(10);
    }
  }

  // 20-21 히터 OFF
  if (isHot) {
    // 히터1 릴레이는 바로 OFF
    Heater1_GPIO_OFF();
    // 히터2 PWM을 완전히 끄고 상태 플래그를 업데이트합니다.
    Heater2_GPIO_OFF();
  }

  // 22. GPIO#18 유량계 총 유량 > 카페라떼 에스프레소 + 우유 설정값 까지 대기
  {
    float targetVol = (float)(a_e_ml_set + a_w_ml_set);
    while (YF_S402B_outputFlow <= targetVol) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      delay(10);
    }
  }

  // 23~26. 기어펌프/밸브 OFF
  GearPump_PWM_OFF();
  expanderWriteForDoc(6, LOW);
  expanderWriteForDoc(5, LOW);
  expanderWriteForDoc(1, LOW);

  // 27. 작업 완료
  createValueReset();
  isWorking = false;
  Serial.println("Create Americano Clear");
};
// ===================================================================
// ===================================================================
// ===================================================================

void createCafeLatte() {
  
  // 2. Start 조건 확인 (isRunning, isWorking), 제조값 초기화
  if (!checkStart()) {
    Serial.println("Create CafeLatte: checkStart is false");
    return;
  } else {
    isWorking = true;
    createValueReset();
    c_esspresso_ml = c_e_ml_set;
    c_milk_ml = c_m_ml_set;
    c_tmp = c_tmp_set;
    Serial.println("Create CafeLatte Start");
  };
  // 3. GPIO Expander #6 ON (에스프레소 출구 바이패스)
  expanderWriteForDoc(1, HIGH);
  // 4. GPIO Expander #9 ON(혼합 입구 펌프)
  expanderWriteForDoc(8, HIGH);
  // 5. GPIO#19 유량계 총 유량 > 카페라떼 에스프레소 설정값까지 대기)
  while (YF_S402B_inputFlow <= (float)c_esspresso_ml) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 6. GPIO Expander #9 OFF(혼합 입구 펌프)
  expanderWriteForDoc(8, LOW);
  // 7. GPIO Expander #1 OFF (에스프레소 출구 바이패스)
  expanderWriteForDoc(1, LOW);
  delay(100);
  // 8. GPIO#19 유량계 누적값 초기화
  YF_S402B_inputFlow = 0;
  // 9. GPIO Expander #3 ON(우유 전자변)
  expanderWriteForDoc(3, HIGH);
  // 10. GPIO Expander #8 On(혼합 입구 펌프)
  expanderWriteForDoc(8, HIGH);
  // 11. GPIO#19 유량계 총 유량 > 카페라떼 우유 설정값 비교값까지 대기
  while (YF_S402B_inputFlow <= (float)c_milk_ml) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  Serial.println("Create CafeLatte 에스프레소, 우유 설정갑 주유 완료");
  // 12. GPIO Expander #3 OFF(우유 전자변)
  expanderWriteForDoc(3, LOW);
  // 13. GPIO Expander #8 OFF(혼합 입구 펌프)
  expanderWriteForDoc(8, LOW);
  delay(100);
  // 14. GPIO Expander #1 On (에스프레소 출구 바이패스)
  expanderWriteForDoc(1, HIGH);
  // 15. GPIO33 PWM ON(기어펌프)
  GearPump_PWM_ON();
  // 16. 기어펌프가 ON된 시간이 설정된 혼합시간과 일치할때까지 대기
  {
    unsigned long startMs = millis();
    unsigned long mixMs = (unsigned long)shake_time * 1000UL;
    while (millis() - startMs < mixMs) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      delay(10);
    }
  }

  Serial.println("Create CafeLatte 혼합 완료");
  // 17. GPIO Expander #6 ON (순환 3Way Valve)
  expanderWriteForDoc(6, HIGH);
  Serial.println("Create CafeLatte 순환끝, 내보내기 시작");
  // 18. 필요한 양의 10%가 이동할 때까지 대기 후 히터 ON
  float totalVol = (float)c_esspresso_ml + (float)c_milk_ml;
  float startVolume = totalVol * 0.1f;
  while (YF_S402B_outputFlow <= startVolume) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
      }
      delay(10);
  }
  // 19. 히터 ON
  if (isHot) {
    Heater1_GPIO_ON();
    Heater2_GPIO_ON();
  }
  Serial.println("Create CafeLatte 10% 이동 완료 히터 켜짐");
  Serial.println("Create CafeLatte 공기 흡입 시작");
  // 
  // 20. 공기 흡입 시작
  bool inhaleOk = startInhaleTask(
    (uint32_t)inhale_w_time * 1000UL,
    (uint32_t)inhale_time   * 1000UL,
    (uint32_t)inhale_on_time  * 1000UL,
    (uint32_t)inhale_off_time * 1000UL,
    7 /* valvePin */, 1 /*prio*/, 1 /*core*/
  );
  if (!inhaleOk) {
    Serial.println("[WARN] InhaleTask start failed");
  }
  

  // 21. 드레인 시간동안 대기
  Serial.println("Create CafeLatte 드레인 시간 대기");
  {
    unsigned long startMs = millis();
    unsigned long drainMs = (unsigned long)drain_time * 1000UL;
    while (millis() - startMs < drainMs) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      // // 히터가 켜진 경우 과열 여부에 따라 ON/OFF를 제어하고 PWM 출력을 갱신합니다.
      if (isHot) {
        // Heater 1: 릴레이 제어
        if (Heater_1_NTC_TEMP >= h1_emer_tmp) {
          Heater1_GPIO_OFF();
        } else {
          Heater1_GPIO_ON();
        }
        // Heater 2: PWM 제어 (PID 출력 사용)
        if (Heater_2_NTC_TEMP >= h2_emer_tmp2) {
          Heater2_GPIO_OFF();
        } else {
          Heater2_GPIO_ON();
        }
      }
      delay(10);
    }
  }
  Serial.println("Create CafeLatte 드레인 시간 완료");
  // 22. GPIO Expander #8 ON(출구 3Way Valve)
  expanderWriteForDoc(5, HIGH);
  // 23. GPIO#18 유량계 총유량 > (카페라떼 에스프레소 설정값 + 카페라떼 우유 설정값) * (히터 정지 유량비율(%) / 100)까지 대기
  {
    YF_S402B_outputFlow = 0;
    float limitVolume = (float)(c_e_ml_set + c_m_ml_set) * ((float)h2_limit_per / 100.0f);
    while (YF_S402B_outputFlow <= limitVolume) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      // 히터가 켜진 경우 과열 여부에 따라 ON/OFF를 제어하고 PWM 출력을 갱신합니다.
      if (isHot) {
        // Heater 1: 릴레이 제어
        if (Heater_1_NTC_TEMP >= h1_emer_tmp) {
          Heater1_GPIO_OFF();
        } else {
          Heater1_GPIO_ON();
        }
        // Heater 2: PWM 제어 (PID 출력 사용)
        if (Heater_2_NTC_TEMP >= h2_emer_tmp2) {
          Heater2_GPIO_OFF();
        } else {
          Heater2_GPIO_ON();
        }
      }
      delay(10);
    }
  }
  
   // 24-25. 히터 1, 히터 2 출력 정지
   if (isHot) {
    // 히터1 릴레이는 바로 OFF
    Heater1_GPIO_OFF();
    // 히터2 PWM을 완전히 끄고 상태 플래그를 업데이트합니다.
    Heater2_GPIO_OFF();
  }
  // 26. GPIO#18 유량계 총 유량 > 카페라떼 에스프레소 + 우유 설정값 까지 대기
  {
    float targetVol = (float)(c_e_ml_set + c_m_ml_set);
    while (YF_S402B_outputFlow <= targetVol) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      delay(10);
    }
  }
  // 27. GPIO32 PWM출력 OFF (기어펌프)
  GearPump_PWM_OFF();
  // 28. GPIO Expander #7 OFF (순환 3Way Valve)
  expanderWriteForDoc(6, LOW);
  // 29. GPIO Expander #8 OFF (출구 3Way Valve)
  expanderWriteForDoc(5, LOW);
  // 30. GPIO Expander #1 OFF (에스프레소 출구 바이패스)
  expanderWriteForDoc(1, LOW);
  // 31. 작업 완료
  createValueReset();
  isWorking = false;
  Serial.println("Create CafeLatte End");
};
// ===================================================================
// ===================================================================
// ===================================================================


// 세정(청소)을 수행하는 함수
void createCleaning() {
  // 2. Start 조건 확인 (isRunning, isWorking), 제조값 초기화
  if (!checkStart()) {
    Serial.println("Create CafeLatte: checkStart is false");
    return;
  } else {
    isWorking = true;
    createValueReset();
    Serial.println("Create CafeLatte Start");
  };

  // 3. GPIO Expander #3 ON(청소 전자변)
  expanderWriteForDoc(4, HIGH);
  // 4. GPIO Expander #9 ON(혼합탱크 입구 펌프)
  expanderWriteForDoc(8, HIGH);

  // 6. 청소 시작후 설정된 청소 전환시간 비교후 시간이 일치하면
  // 시간 측정 시작
  unsigned long startMs1 = millis();
  unsigned long switchMs = (unsigned long)clean_time * 1000UL;
  unsigned long totalMs = (unsigned long)clean_time_all * 1000UL;

  // 1차 청소 전환 시점까지 대기
  while (millis() - startMs1 < switchMs) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 물이 들어왔으니 밸브랑 펌프닫기
  // 7. GPIO Expander #3 OFF(청소 전자변)
  expanderWriteForDoc(4, LOW);
  // 8. GPIO Expander #9 OFF(혼합탱크 입구 펌프)
  expanderWriteForDoc(8, LOW);

    // 물이 들어왔으니 펌프 켜기
    GearPump_PWM_ON();
    // 1차 청소 전환 시점까지 대기
  unsigned long startMs2 = millis();
  while (millis() - startMs2 < switchMs) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  //
  expanderWriteForDoc(6, HIGH);
  // 
  unsigned long startMs3 = millis();
  while (millis() - startMs3 < totalMs) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 12. GPIO33 OFF(기어펌프)
  GearPump_PWM_OFF();
  // 13. GPIO Expander #6 OFF(혼합 3Way Valve)
  expanderWriteForDoc(6, LOW);
  // 17. 작업 완료
  // 작업 완료
  createValueReset();
  isWorking = false;
  Serial.println("Cleaning Clear");
};
// FreeRTOS 태스크 래퍼 함수들
void espressoTask(void *parameter) {
  createEspresso();
  vTaskDelete(NULL);
};
void americanoTask(void *parameter) {
  createAmericano();
  vTaskDelete(NULL);
};
void cafelatteTask(void *parameter) {
  createCafeLatte();
  vTaskDelete(NULL);
};
void cleaningTask(void *parameter) {
  createCleaning();
  vTaskDelete(NULL);
};



