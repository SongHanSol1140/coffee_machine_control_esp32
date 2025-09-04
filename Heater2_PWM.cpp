// Heater2_PWM.cpp
#include <Arduino.h>
#include "variables.h"
#include "Heater2_PWM.h"

// ===== 원하는 스타일 선택 =====
// 핀 기반 래퍼(ledcAttach(pin, freq, res) & ledcWrite(pin, duty)) 사용 시 켜세요.

// ===== 공통 설정 =====
static constexpr int HEATER2_PWM_FREQ_HZ  = 12000; // Hz
static constexpr int HEATER2_RES_BITS     = 8;   // 10-bit -> duty 0~1023 / 8-bit -> duty 0~255
static constexpr int HEATER2_MAX_DUTY     = (1 << HEATER2_RES_BITS) - 1;


// ===== PID 내부 상태 =====
static float integralTerm = 0.0f;
static float prevError    = 0.0f;
static unsigned long lastMs = 0;
static constexpr float INTEGRAL_CLAMP = 1000.0f;

static inline bool heaterShouldBeOff() {
  if (emergencyStop) return true;
  return false;
}
void Heater1_GPIO_Setup(){
  pinMode(Heater_1_GPIO_PIN, OUTPUT);
  digitalWrite(Heater_1_GPIO_PIN, LOW);
  delay(100);
}
void Heater2_PWM_Setup() {
	ledcAttach(Heater_2_PWM_PIN, HEATER2_PWM_FREQ_HZ, HEATER2_RES_BITS);
	ledcWrite(Heater_2_PWM_PIN, 0);

	integralTerm = 0.0f;
	prevError    = 0.0f;
	lastMs       = millis();
	Heater_2_PWM_output_value = 0;
	delay(100);
}

void Heater2_PWM_Compute() {
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  if (dt <= 0.0f) dt = 1e-3f;
  lastMs = now;

  const float setpoint = (float)create_temperature;
  const float current  = Heater_2_NTC_TEMP;
  float error = setpoint - current;

  float P = (float)Heter_PID_P * error;
  integralTerm += (float)Heter_PID_I * error * dt;
  if (integralTerm >  INTEGRAL_CLAMP) integralTerm =  INTEGRAL_CLAMP;
  if (integralTerm < -INTEGRAL_CLAMP) integralTerm = -INTEGRAL_CLAMP;
  float derivative = (error - prevError) / dt;
  float D = (float)Heter_PID_D * derivative;

  float u = P + integralTerm + D;
  prevError = error;

  int duty = (int)round(u);
  if (duty < 0) duty = 0;
  if (duty > HEATER2_MAX_DUTY) duty = HEATER2_MAX_DUTY;

  if (heaterShouldBeOff()) duty = 0;

  Heater_2_PWM_output_value = duty;
}

void Heater2_PWM_Write() {
  int duty =Heater_2_PWM_output_value;
  ledcWrite(Heater_2_PWM_PIN, duty);
}

void Heater2_PWM_ForceOff() {
  Heater_2_PWM_output_value = 0;
  ledcWrite(Heater_2_PWM_PIN, 0);
}