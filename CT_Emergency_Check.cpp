// CT_Emergency_Check.cpp
#include <Arduino.h>
#include "CT_Emergency_Check.h"
#include "variables.h"

const int NUM_READS = 5;       // 평균을 낼 읽기 횟수
// 중요: 전류가 흐르지 않을 때 측정되는 ADC 값을 여기에 입력하세요.
// 이 값은 센서마다, 전원 환경에 따라 조금씩 다릅니다.
void CT_Emergency_Check() {
  long sum = 0;
  for (int i = 0; i < NUM_READS; i++) {
    sum += analogRead(CT_emergencyAmpere_check_PIN);
    delay(2);
  }
  ctAnalogValue =  sum / NUM_READS;
  
  currentAmpere = (ctAnalogValue - ctAdcZero) * (3.3 / 4095.0) / 0.185;

  if (currentAmpere > emergencyA) {
    emergencyStop = true;
  }
}