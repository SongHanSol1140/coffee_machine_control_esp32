// CT_Emergency_Check.cpp
#include <Arduino.h>
#include "CT_Emergency_Check.h"
#include "variables.h"

const int NUM_READS = 5;       // 평균을 낼 읽기 횟수
// 중요: 전류가 흐르지 않을 때 측정되는 ADC 값을 여기에 입력하세요.
// 이 값은 센서마다, 전원 환경에 따라 조금씩 다릅니다.
const int adc_zero = 1950;
void CT_Emergency_Check() {
  long sum = 0;
  for (int i = 0; i < NUM_READS; i++) {
    sum += analogRead(CT_emergencyAmpere_check_PIN);
    delay(2);
  }
  float analogValue =  sum / NUM_READS;
  currentAmpere = (analogValue - adc_zero) * (3.3 / 4095.0) / 0.185;
	// 아날로그값(평균)
	Serial.print("analog value: ");
	Serial.println(analogValue);
	// VCC로 환산
	float vcc = analogValue * 3.3 / 4095;
	Serial.print("VCC: ");
	Serial.println(vcc);
	//   // Ampere (20A ACS712)
	//   // (현재 ADC값 - 0A일때 ADC값) * (ADC를 전압으로 바꾸는 비율) / (센서 감도)
	//   float ampere = (analogValue - adc_zero) * (3.3 / 4095.0) / 0.1;
	//   Serial.print("Ampere: ");
	//   Serial.println(ampere);

}