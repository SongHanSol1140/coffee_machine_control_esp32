// NTC_TempertureSensor.cpp
#include <Arduino.h>
#include <math.h>
#include "variables.h"
#include "NTC_TempertureSensor.h"

// ===== 사용자/하드웨어 설정 =====
// (3.3V측) 고정저항
static constexpr float SERIES_RESISTOR     = 10000.0f;  // Ω
// NTC 정격값(25°C에서의 저항)
static constexpr float NOMINAL_RESISTANCE  = 50000.0f;  // Ω @ 25°C
// NTC 정격 온도
static constexpr float NOMINAL_TEMPERATURE = 25.0f;     // °C
// 베타 계수 (데이터시트 B값)
static constexpr float BETA_COEFFICIENT    = 3976.0f;
// ESP32 ADC 설정
static constexpr int   ADC_MAX             = 4095;      // 12-bit
static constexpr int   SAMPLES             = 5;        // 샘플 평균

// -------- 내부 유틸 --------
static float readADCavg(int pin) {
  uint32_t sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return (float)sum / (float)SAMPLES;
}

static float ntcResistanceFromADC(float adc) {
  // 배선: 위=고정저항, 아래=NTC, 분압점=ADC
  // Vout = Vcc * (R_ntc / (R_fixed + R_ntc))
  // => R_ntc = R_fixed * (adc / (ADC_MAX - adc))
  if (adc <= 0.0f) return NAN;
  if (adc >= (float)ADC_MAX) return NAN;
  return SERIES_RESISTOR * (adc / ((float)ADC_MAX - adc));
}

static float ntcToCelsius(float r_ntc) {
  if (!isfinite(r_ntc) || r_ntc <= 0.0f) return NAN;

  // 베타식
  float t0   = NOMINAL_TEMPERATURE + 273.15f;       // K
  float invT = (1.0f / t0) + (1.0f / BETA_COEFFICIENT) * logf(r_ntc / NOMINAL_RESISTANCE);
  float tempK = 1.0f / invT;
  return tempK - 273.15f;
}

// -------- 공개 API --------
void NTC_Temperture_Setup() {
  // ESP32 ADC 기본 설정
  analogReadResolution(12);
  // 11dB 감쇠: 약 3.3V 범위(≈ 150~245 LSB 오차 범위 내)까지 측정
  analogSetPinAttenuation(Heater_1_NTC_PIN, ADC_11db);
  analogSetPinAttenuation(Heater_2_NTC_PIN, ADC_11db);

  // 첫 프린트
  Serial.println(F("[NTC] Setup complete (Top=Fixed 10k, Bottom=NTC)"));
  delay(100);
}

void heater_1_NTC_Temperture_Read() {
  // Heater 1
  float adc1   = readADCavg(Heater_1_NTC_PIN);
  float r1     = ntcResistanceFromADC(adc1);
  float temp1C = ntcToCelsius(r1);
  // 결과 업데이트(전역 변수)
  Heater_1_NTC_TEMP = temp1C;

  // 기본 유효성 체크
  if (adc1 <= 0.5f) {
	Heater_1_NTC_TEMP = -1.0f;
  } else if (adc1 >= (ADC_MAX - 0.5f)) {
    Heater_1_NTC_TEMP = -2.0f;
  }
}
void heater_2_NTC_Temperture_Read() {
  float adc2   = readADCavg(Heater_2_NTC_PIN);
  float r2     = ntcResistanceFromADC(adc2);
  float temp2C = ntcToCelsius(r2);

  // 결과 업데이트(전역 변수)
  Heater_2_NTC_TEMP = temp2C;

  // 기본 유효성 체크
  if (adc2 <= 0.5f) {
    Heater_2_NTC_TEMP = -1.0f;
  } else if (adc2 >= (ADC_MAX - 0.5f)) {
    Heater_2_NTC_TEMP = -2.0f;
  }

}
