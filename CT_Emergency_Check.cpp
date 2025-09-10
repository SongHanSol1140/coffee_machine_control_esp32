// CT_Emergency_Check.cpp
#include <Arduino.h>
#include "CT_Emergency_Check.h"
#include "variables.h"
#include <math.h>
const int NUM_READS = 5;       // 평균 값을 낼 횟수
float volts_per_count = 3.3f / 4095.0f;   // f 붙여서 float 유지
float amps_per_volt  = 1.0f / 0.185f;


// 중요: 전류가 흐르지 않을 때 측정되는 ADC 값을 여기에 입력하세요.
// 이 값은 센서마다, 전원 환경에 따라 조금씩 다릅니다.

void CT_Emergency_Check() {
  long sum = 0;
  for (int i = 0; i < NUM_READS; i++) {
    sum += analogRead(CT_emergencyAmpere_check_PIN);
    delay(2);
  }
  ctAnalogValue =  sum / NUM_READS;
  
  float v = ((float)ctAnalogValue - (float)ctAdcZero) * volts_per_count * amps_per_volt;
    currentAmpere = fabsf(v);  // 절대값 적용 (음수→양수)

  if (currentAmpere > emergencyA) {
    emergencyStop = true;
  }
}