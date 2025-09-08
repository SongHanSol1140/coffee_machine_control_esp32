// Heater2_PWM.cpp
#include <Arduino.h>
#include "variables.h"
#include "Heater.h"
// =====================================================
/*PID 적정값 메모
공용 추천 값 - P = 5.5, I = 0.6, D = 25
 0)목표 : 80도 - P = 6.5, I = 0,8, D = 30
 1)목표 : 70도 - P = 6, I = 0,7, D = 28
 2)목표 : 60도 - P = 5.5, I = 0,6, D = 25
 3)목표 : 50도 - P = 5, I = 0,5, D = 22
 4)목표 : 40도  - P = 4, I = 0.5, D = 20

 요구사항 : 목표 온도에 도달시 급격하게 온도가 올라가지 않도록 해야함 => 출력을 상당히 많이떨어트려야함
*/
// =====================================================

// ===== 공통 설정 =====
static constexpr int HEATER2_PWM_FREQ_HZ  = 60; // Hz
static constexpr int HEATER2_RES_BITS     = 8;   // 10-bit -> duty 0~1023 / 8-bit -> duty 0~255
static constexpr int HEATER2_MAX_DUTY     = (1 << HEATER2_RES_BITS) - 1;

// ===== PID 내부 상태 =====
static constexpr int    MA_SIZE      = 10;      // 이동평균 길이
static float            errorBuf[MA_SIZE] = {0};
static int              bufIndex = 0;
static float            errorSum = 0.0f;
static float            prevAvgError = 0.0f;
static float            integralTerm = 0.0f;

static unsigned long lastMs = 0;

// 조건부 적분 클램프
static constexpr float INTEGRAL_CLAMP = 255.0f;




// =====================================================
// =====================================================
static inline bool heaterOverheated() {
  if (emergencyStop) return true;
  if (Heater_2_NTC_TEMP >= h2_emer_tmp2) return true;
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
	delay(100);

  // 기본 상태는 PWM 출력이 꺼진 것으로 간주합니다.
  // ledcAttach/ledcWrite를 통해 0 duty로 초기화하지만 상태 플래그를 명시적으로 false로
  // 설정하여 웹 UI 등에서 정확한 초기 상태를 표시할 수 있게 합니다.
	Heater_2_PWM_output_value = 0;
  heater2_On = false;
}

/**
 * PID 계산: 조건부 적분 + Dirty Derivative
 * Heater_2_PWM_output_value 에 0~255 사이의 듀티를 저장한다.
 */
void Heater2_PWM_Compute() {
  // 시간
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  if (dt <= 0.0001f) dt = 0.0001f;
  lastMs = now;

  // 목표/현재/오차
  const float setpoint = (float)c_tmp;
  const float current  = Heater_2_NTC_TEMP;
  float error = setpoint - current;

  // === 이동평균(오차 완화) ===
  errorSum -= errorBuf[bufIndex];
  errorBuf[bufIndex] = error;
  errorSum += error;
  bufIndex = (bufIndex + 1) % MA_SIZE;

  float avgError = errorSum / MA_SIZE;
  static float prevAvgError = 0.0f;
  float dError = (avgError - prevAvgError) / dt;
  prevAvgError = avgError;

  // === PID 원시 출력 ===
  float P = (float)Heter_PID_P * avgError;

  // --- 적분: 일단 후보 계산 ---
  float I_candidate = integralTerm + (float)Heter_PID_I * avgError * dt;

  float D = (float)Heter_PID_D * dError;

  // 랜딩 밴드: 목표 접근 시 의도적 감속
  static const float LANDING_BAND = 5.0f; // ℃
  float taper = 1.0f;
  if (error <= 0.0f) {
    taper = 0.0f;                 // 목표 이상이면 바로 0
  } else if (error < LANDING_BAND) {
    taper = error / LANDING_BAND; // 0~1
  }

  // 후보 출력 (적분 바람 포함) + 테이퍼
  float u_unsat = (P + I_candidate + D);
  if (u_unsat < 0.0f) u_unsat = 0.0f;
  if (u_unsat > HEATER2_MAX_DUTY) u_unsat = HEATER2_MAX_DUTY;
  u_unsat *= taper; // 접근 시 강제 감속

  // === 안티 윈드업: 조건부 적분 ===
  // 상포화(255) & 양오차 → 적분 중단, 하포화(0) & 음오차 → 적분 중단
  bool atUpper = (u_unsat >= (float)HEATER2_MAX_DUTY - 1.0f);
  bool atLower = (u_unsat <= 1.0f);
  if (!((atUpper && avgError > 0.0f) || (atLower && avgError < 0.0f))) {
    integralTerm = I_candidate;
  }
  // 적분 클램프
  if (integralTerm >  INTEGRAL_CLAMP) integralTerm =  INTEGRAL_CLAMP;
  if (integralTerm < -INTEGRAL_CLAMP) integralTerm = -INTEGRAL_CLAMP;

  // === 목표 이상일 때 적분 빨리 빼기(bleed) ===
  if (error <= 0.0f && integralTerm > 0.0f) {
    // half-life ≈ 0.5s (원하면 0.3~1.0 로 바꿔 조정)
    float decay = powf(0.5f, dt / 0.5f);
    integralTerm *= decay;
    if (integralTerm < 1e-3f) integralTerm = 0.0f;
  }

  // 최종 출력 재계산(테이퍼 포함)
  float u = (float)Heter_PID_P * avgError + integralTerm + (float)Heter_PID_D * dError;
  if (u < 0.0f) u = 0.0f;
  if (u > (float)HEATER2_MAX_DUTY) u = (float)HEATER2_MAX_DUTY;
  u *= taper;

  // 안전 게이트
  if (heaterOverheated()) u = 0.0f;

  Heater_2_PWM_output_value = (int)lroundf(u);
}// =====================================================
// =====================================================
void Heater1_GPIO_On() {
  heater1_On = true;
};
void Heater1_GPIO_Off() {
  heater1_On = false;
};
void Heater2_PWM_On() {
  heater2_On = true;
};
void Heater2_PWM_Off() {
  heater2_On = false;
};

// 사용안함. .ino에서 

void Heater1_GPIO_Write() {
  if(!heater1_On || heaterOverheated()){
    digitalWrite(Heater_1_GPIO_PIN, LOW);
  }else{
    digitalWrite(Heater_1_GPIO_PIN, HIGH);
  }
};
void Heater2_PWM_Write() {
  if(!heater2_On || heaterOverheated()){
    ledcWrite(Heater_2_PWM_PIN, 0);
  }else{
    ledcWrite(Heater_2_PWM_PIN, Heater_2_PWM_output_value);
  }
};

