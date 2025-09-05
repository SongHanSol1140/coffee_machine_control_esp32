// machineRunning.cpp
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "variables.h"
#include "machineRunning.h"

// GPIO 확장자와 기어펌프 제어 함수 사용을 위해 헤더 포함
#include "MCP23017.h"
#include "GearPump_PWM.h"

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
  // 모든 출력을 끕니다. 비상 상황에서 호출되므로 현재 진행 중인 작업을 완전히 정지합니다.
  // 기어펌프 및 히터 OFF
  GearPump_PWM_OFF();
  if (isHot) {
    digitalWrite(Heater_1_GPIO_PIN, LOW);
    ledcWrite(Heater_2_PWM_PIN, 0);
  }
  // 모든 expander 핀을 LOW로 설정하여 솔레노이드 밸브/펌프/밸브를 종료합니다.
  for (int pin = 1; pin <= 10; ++pin) {
    expanderWriteForDoc(pin, LOW);
  }
  // 시스템 상태 플래그 리셋
  isRunning = false;
  isWorking = false;
  emergencyStop = false;
}

void createValueReset() {
  YF_S402B_outputFlow = 0;
  YF_S402B_inputFlow = 0;
  c_esspresso_ml = 0;
  c_water_ml = 0;
  c_milk_ml = 0;
  c_tmp = 0;
}

void createEspresso() {
  if (!checkStart()) {
    Serial.println("Create Espresso: checkStart is false");
    return;
  } else {
    isWorking = true;
    createValueReset();
    c_esspresso_ml = e_ml_set;
    c_tmp = e_tmp_set;
    Serial.println("Create Espresso Start");
  };


  // GPIO Expander #7 High (추출 구동 시작)
  expanderWriteForDoc(7, HIGH);
  // 히터 제어: isHot인 경우에만 켜고, isCold이면 히터를 사용하지 않음
  if (isHot) {
    digitalWrite(Heater_1_GPIO_PIN, HIGH);
    // 히터2 PWM 출력 시작 (PID에 의해 계산된 값 사용)
    ledcWrite(Heater_2_PWM_PIN, Heater_2_PWM_output_value);
  };
  // 기어펌프 PWM 켜기
  GearPump_PWM_ON();

  Serial.println("Tast Check 2");
  // 드레인 시간만큼 대기 (ms) 동안 비상 상황 체크
  unsigned long startMs = millis();
  unsigned long drainMs = (unsigned long)drain_time * 1000UL;
  while (millis() - startMs < drainMs) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }

  // GPIO Expander #8 High (추출 완료 후 배출 시작)
  expanderWriteForDoc(8, HIGH);
  Serial.println("Tast Check 3");
  // 유량계 #1 출구 값이 limit 퍼센트까지 도달할 때까지 대기
  float limitVolume = (float)e_ml_set * ((float)h2_limit_per / 100.0f);
  while (YF_S402B_outputFlow <= limitVolume) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 히터 1, 히터 2 출력 정지
  if (isHot) {
    digitalWrite(Heater_1_GPIO_PIN, LOW);
    ledcWrite(Heater_2_PWM_PIN, 0);
  }

  Serial.println("Tast Check 4");
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
  expanderWriteForDoc(7, LOW);
  expanderWriteForDoc(8, LOW);

  // 제조 완료 상태
  isWorking = false;
  Serial.println("Create Espresso Clear");
};
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
  expanderWriteForDoc(6, HIGH);
  // 4. 혼합 입구 펌프 ON
  expanderWriteForDoc(9, HIGH);

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
  expanderWriteForDoc(9, LOW);
  // 7. 입력 유량계 누적값 초기화
  YF_S402B_inputFlow = 0;

  // 8. 정수 물 전자변 ON
  expanderWriteForDoc(4, HIGH);

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
  expanderWriteForDoc(4, LOW);

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

  // 13~14. 히터 작동 (핫 모드일 때만)
  if (isHot) {
    digitalWrite(Heater_1_GPIO_PIN, HIGH);                   // 릴레이
    ledcWrite(Heater_2_PWM_PIN, Heater_2_PWM_output_value);  // SSR(PWM, PID로 갱신)
  }

  // 15. 공기 흡입 로직 (대기 → ON/OFF 반복)
  {
    // 시작 대기
    unsigned long startMs = millis();
    unsigned long waitMs = (unsigned long)inhale_w_time * 1000UL;
    while (millis() - startMs < waitMs) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      delay(10);
    }

    // ON/OFF 반복
    unsigned long totalMs = (unsigned long)inhale_time * 1000UL;
    unsigned long cycleStart = millis();
    while (millis() - cycleStart < totalMs) {
      // ON
      unsigned long onMs = (unsigned long)inhale_on_time * 1000UL;
      unsigned long remaining = totalMs - (millis() - cycleStart);
      if (onMs > remaining) onMs = remaining;

      expanderWriteForDoc(5, HIGH);
      unsigned long tStart = millis();
      while (millis() - tStart < onMs) {
        if (emergencyStop || currentAmpere > emergencyA) {
          Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
          stopAllOutputsAndResetFlags();
          return;
        }
        delay(10);
      }

      // OFF
      expanderWriteForDoc(5, LOW);
      unsigned long offMs = (unsigned long)inhale_off_time * 1000UL;
      remaining = totalMs - (millis() - cycleStart);
      if (offMs > remaining) offMs = remaining;

      tStart = millis();
      while (millis() - tStart < offMs) {
        if (emergencyStop || currentAmpere > emergencyA) {
          Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
          stopAllOutputsAndResetFlags();
          return;
        }
        delay(10);
      }
    }
    // 종료 시 OFF 보장
    expanderWriteForDoc(5, LOW);
  }

  // 16. 순환 3Way Valve ON
  expanderWriteForDoc(7, HIGH);

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
      delay(10);
    }
  }

  // 18. 출구 3Way Valve ON
  expanderWriteForDoc(8, HIGH);

  // 19. 히터 정지 유량비율까지 대기
  {
    float limitVolume = (float)(a_e_ml_set + a_w_ml_set) * ((float)h2_limit_per / 100.0f);
    while (YF_S402B_outputFlow <= limitVolume) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      delay(10);
    }
  }

  // 20. 히터 OFF
  if (isHot) {
    digitalWrite(Heater_1_GPIO_PIN, LOW);
    ledcWrite(Heater_2_PWM_PIN, 0);
  }

  // 21. 목표량까지 배출 대기
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

  // 22~25. 기어펌프/밸브 OFF
  GearPump_PWM_OFF();
  expanderWriteForDoc(7, LOW);
  expanderWriteForDoc(8, LOW);
  expanderWriteForDoc(6, LOW);

  // 26. 작업 완료
  isWorking = false;
  Serial.println("createAmericano End");
};
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
  expanderWriteForDoc(6, HIGH);
  // 4. GPIO Expander #9 ON(혼합 입구 펌프)
  expanderWriteForDoc(9, HIGH);
  // 5. GPIO#19 유량계 총 유량 > 카페라떼 에스프레소 설정값까지 대기)
  while (YF_S402B_inputFlow <= (float)c_e_ml_set) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 6. GPIO Expander #9 OFF(혼합 입구 펌프)
  expanderWriteForDoc(9, LOW);
  // 7. GPIO#19 유량계 누적값 초기화
  YF_S402B_inputFlow = 0;
  // 8. GPIO Expander #2 ON(우유 전자변)
  expanderWriteForDoc(2, HIGH);
  // 9. GPIO#19 유량계 총 유량 > 카페라떼 우유 설정값 비교값까지 대기
  while (YF_S402B_inputFlow <= (float)c_m_ml_set) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 10. GPIO Expander #2 OFF(우유 전자변)
  expanderWriteForDoc(2, LOW);

  // 11. GPIO33 PWM ON(기어펌프)
  GearPump_PWM_ON();
  // 12. GPIO33 PWM ON된 시간이 설정된 혼합시간과 일치할때까지 대기
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

  // 13-14 히터 ON (핫 모드일 때)
  if (isHot) {
    digitalWrite(Heater_1_GPIO_PIN, HIGH);
    ledcWrite(Heater_2_PWM_PIN, Heater_2_PWM_output_value);
  }

  // 15. 공기 흡입 시작
  {
    unsigned long startMs = millis();
    unsigned long waitMs = (unsigned long)inhale_w_time * 1000UL;
    while (millis() - startMs < waitMs) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      delay(10);
    }
  }
  // 공기 흡입 ON/OFF 반복
  {
    unsigned long totalMs = (unsigned long)inhale_time * 1000UL;
    unsigned long cycleStart = millis();
    while (millis() - cycleStart < totalMs) {
      unsigned long onMs = (unsigned long)inhale_on_time * 1000UL;
      unsigned long remaining = totalMs - (millis() - cycleStart);
      if (onMs > remaining) onMs = remaining;
      expanderWriteForDoc(5, HIGH);
      unsigned long tStart = millis();
      while (millis() - tStart < onMs) {
        if (emergencyStop || currentAmpere > emergencyA) {
          Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
          stopAllOutputsAndResetFlags();
          return;
        }
        delay(10);
      }
      expanderWriteForDoc(5, LOW);
      unsigned long offMs = (unsigned long)inhale_off_time * 1000UL;
      remaining = totalMs - (millis() - cycleStart);
      if (offMs > remaining) offMs = remaining;
      tStart = millis();
      while (millis() - tStart < offMs) {
        if (emergencyStop || currentAmpere > emergencyA) {
          Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
          stopAllOutputsAndResetFlags();
          return;
        }
        delay(10);
      }
    }
    expanderWriteForDoc(5, LOW);
  }
  // 16. GPIO Expander #7 ON (순환 3Way Valve)
  expanderWriteForDoc(7, HIGH);
  // 17. 드레인 시간동안 대기
  {
    unsigned long startMs = millis();
    unsigned long drainMs = (unsigned long)drain_time * 1000UL;
    while (millis() - startMs < drainMs) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      delay(10);
    }
  }
  // 18. GPIO Expander #8 ON(출구 3Way Valve)
  expanderWriteForDoc(8, HIGH);
  // 19. GPIO#18 유량계 총유량 > (카페라떼 에스프레소 설정값 + 카페라떼 우유 설정값) * (히터 정지 유량비율(%) / 100)까지 대기
  {
    float limitVolume = (float)(c_e_ml_set + c_m_ml_set) * ((float)h2_limit_per / 100.0f);
    while (YF_S402B_outputFlow <= limitVolume) {
      if (emergencyStop || currentAmpere > emergencyA) {
        Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
        stopAllOutputsAndResetFlags();
        return;
      }
      delay(10);
    }
  }
  // 20-21. 히터 OFF
  if (isHot) {
    digitalWrite(Heater_1_GPIO_PIN, LOW);
    ledcWrite(Heater_2_PWM_PIN, 0);
  }
  // 22. GPIO#18 유량계 총 유량 > 카페라떼 에스프레소 + 우유 설정값 까지 대기
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
  // 23. GPIO32 PWM출력 OFF (기어펌프)
  GearPump_PWM_OFF();
  // 24. GPIO Expander #7 OFF (순환 3Way Valve)
  expanderWriteForDoc(7, LOW);
  // 25. GPIO Expander #8 OFF (출구 3Way Valve)
  expanderWriteForDoc(8, LOW);
  // 26. GPIO Expander #6 OFF (에스프레소 출구 바이패스)
  expanderWriteForDoc(6, LOW);
  // 27. 작업 완료
  isWorking = false;
  Serial.println("Create CafeLatte End");
};
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
  expanderWriteForDoc(3, HIGH);
  // 4. GPIO Expander #9 ON(혼합탱크 입구 펌프)
  expanderWriteForDoc(9, HIGH);
  // 5. GPIO33 ON(기어펌프)
  GearPump_PWM_ON();
  // 6. 청소 시작후 설정된 청소 전환시간 비교후 시간이 일치하면
  // 시간 측정 시작
  unsigned long startMs = millis();
  unsigned long switchMs = (unsigned long)clean_time * 1000UL;
  unsigned long totalMs = (unsigned long)clean_time_all * 1000UL;

  // 1차 청소 전환 시점까지 대기
  while (millis() - startMs < switchMs) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 7. GPIO Expander #3 OFF(청소 전자변)
  expanderWriteForDoc(3, LOW);
  // 8. GPIO Expander #9 OFF(혼합탱크 입구 펌프)
  expanderWriteForDoc(9, LOW);
  // 9. GPIO Expander #7 ON(혼합 3Way Valve)
  expanderWriteForDoc(7, HIGH);
  // 10. GPIO Expander #8 ON(출구 3Way Valve)
  expanderWriteForDoc(8, HIGH);

  // 11. 청소 시작후 설정된 청소 전체 시간 비교후 시간이 일치하면
  while (millis() - startMs < totalMs) {
    if (emergencyStop || currentAmpere > emergencyA) {
      Serial.println(emergencyStop ? "Emergency Stop" : "Current Ampere Over");
      stopAllOutputsAndResetFlags();
      return;
    }
    delay(10);
  }
  // 12. GPIO33 OFF(기어펌프)
  GearPump_PWM_OFF();
  // 13. GPIO Expander #7 OFF(혼합 3Way Valve)
  expanderWriteForDoc(7, LOW);
  // 14. GPIO Expander #8 OFF(출구 3Way Valve)
  expanderWriteForDoc(8, LOW);
  // 15. GPIO Expander #3 OFF(청소 전자변)
  expanderWriteForDoc(3, LOW);
  // 16. 작업 완료
  // 작업 완료
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
