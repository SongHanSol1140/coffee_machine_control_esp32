// machineRunning.cpp
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "variables.h"
#include "machineRunning.h"

// GPIO 확장자와 기어펌프 제어 함수 사용을 위해 헤더 포함
#include "MCP23017.h"
#include "GearPump_PWM.h"


void stopAllOutputsAndResetFlags() {
  GearPump_PWM_OFF();
  if(isHot) {
    digitalWrite(Heater_1_GPIO_PIN, LOW);
    ledcWrite(Heater_2_PWM_PIN, 0);
  }
  expanderWriteForDoc(7, LOW);
  expanderWriteForDoc(8, LOW);
  isRunning = false;
  isWorking = false;
}

// 에스프레소 제조를 수행하는 함수
// isRunning 플래그가 true인 경우에만 동작하며, 이미 제조 중(isWorking==true)인 경우 바로 반환한다.
// 제작 과정에서 currentAmpere > emergencyA 또는 emergencyStop 상태가 되면 즉시 모든 출력을 끄고 종료한다.
// 제조 완료 후 isWorking 플래그를 false로 되돌린다.
void createEspresso() {
  // 전제조건: 시스템이 실행 상태여야 함
  if (!isRunning) {
    Serial.println("createEspresso: isRunning is false");
    return;
  }
  // 다른 제조가 진행 중이면 새로 시작할 수 없음
  if (isWorking) {
    Serial.println("createEspresso: isWorking is true");
    return;
  }

  Serial.println("Tast Check 1");
  // 제조 시작 상태 설정
  isWorking = true;


  
  // 커피 제조용 변수 초기화 후 에스프레소 설정값으로 설정
  c_esspresso_ml = 0;
  c_water_ml     = 0;
  c_milk_ml      = 0;
  c_tmp          = 0;
  // 에스프레소는 우유와 물을 사용하지 않음. 필요한 경우 여기서 수정
  c_esspresso_ml = e_ml_set;
  // c_water_ml, c_milk_ml, c_tmp는 0으로 유지

  // GPIO Expander #7 High (추출 구동 시작)
  expanderWriteForDoc(7, HIGH);
  // 히터 제어: isHot인 경우에만 켜고, isCold이면 히터를 사용하지 않음
  if (isHot) {
    digitalWrite(Heater_1_GPIO_PIN, HIGH);
    // 히터2 PWM 출력 시작 (PID에 의해 계산된 값 사용)
    ledcWrite(Heater_2_PWM_PIN, Heater_2_PWM_output_value);
  }
  // 기어펌프 PWM 켜기
  GearPump_PWM_ON();

  Serial.println("Tast Check 2");
  // 드레인 시간만큼 대기 (ms) 동안 비상 상황 체크
  unsigned long startMs = millis();
  unsigned long drainMs = (unsigned long)drain_time * 1000UL;
  while (millis() - startMs < drainMs) {
    // 비상 정지 또는 과전류 시 즉시 중지
    if (emergencyStop || currentAmpere > emergencyA) {
      // 모든 출력 OFF
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
      GearPump_PWM_OFF();
      if (isHot) {
        digitalWrite(Heater_1_GPIO_PIN, LOW);
        ledcWrite(Heater_2_PWM_PIN, 0);
      }
      expanderWriteForDoc(7, LOW);
      expanderWriteForDoc(8, LOW);
      isWorking = false;
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
  Serial.println("Tast Check Clear!");
}


// FreeRTOS 태스크 함수: createEspresso 호출 후 태스크 종료
void espressoTask(void *parameter) {
  createEspresso();
  // 태스크 핸들 지워줌
  vTaskDelete(NULL);
}
