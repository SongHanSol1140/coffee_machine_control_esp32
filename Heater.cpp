// Heater2_PWM.cpp
#include <Arduino.h>
#include "variables.h"
#include "Heater.h"

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

  // 기본 상태는 PWM 출력이 꺼진 것으로 간주합니다.
  // ledcAttach/ledcWrite를 통해 0 duty로 초기화하지만 상태 플래그를 명시적으로 false로
  // 설정하여 웹 UI 등에서 정확한 초기 상태를 표시할 수 있게 합니다.
  heater2On = false;
}

void Heater2_PWM_Compute() {
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  if (dt <= 0.0f) dt = 1e-3f;
  lastMs = now;

  const float setpoint = (float)c_tmp;
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
  int duty = Heater_2_PWM_output_value;
  // heater2On 플래그가 false인 경우에는 계산된 duty를 출력하지 않고 강제로 0을 내보냅니다.
  // 이를 통해 사용자가 PWM 출력 OFF를 선택했을 때 주기적으로 호출되는 PWM_Write가 다시 출력을 켜지 않도록 합니다.
  if (!heater2On) {
    ledcWrite(Heater_2_PWM_PIN, 0);
    // off 상태에서는 플래그를 유지합니다.
    return;
  }

  // ON 상태이면 계산된 duty 값을 그대로 출력합니다.
  ledcWrite(Heater_2_PWM_PIN, duty);
  // 출력 duty가 0인 경우 자동으로 플래그를 false로 갱신합니다.
  heater2On = (duty > 0);
}

void Heater2_PWM_ForceOff() {
  Heater_2_PWM_output_value = 0;
  ledcWrite(Heater_2_PWM_PIN, 0);

  // 강제 OFF 시 상태 플래그도 업데이트합니다.
  heater2On = false;
}