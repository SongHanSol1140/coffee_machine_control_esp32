// Heater2.cpp
#include <Arduino.h>
#include <math.h>
#include <limits.h>
#include "variables.h"
#include "Heater.h"
#include <driver/gpio.h>
#include <esp_timer.h>
#include <PID_v1.h>
// =====================================================
// 공통/보조
// =====================================================
static inline bool heater1_Overheated() {
  if (emergencyStop) return true;
  if (Heater_1_NTC_TEMP >= h1_emer_tmp) return true;
  return false;
}
static inline bool heater2_Overheated() {
  if (emergencyStop) return true;
  if (Heater_2_NTC_TEMP >= h2_emer_tmp2) return true;
  return false;
}
static void Heater1_ForceOff();
// =====================================================
// Heater1 요구사항: ISR(타이머 콜백) 기반 윈도우 제어
// - 주기(ms)는 가변
// - 매 주기 시작 시 PID 먼저 계산 => 그 결과(0~100%)만큼 ON 유지 후 OFF
// - 전원 출력조건 = heater2_On && !heater1_Overheated() 를 만족할 때만 ON
// - 출력 계산(PID)은 항상 수행 (출력 여부와 별도로, 출력은 플래그/안전 조건으로 결정)
//
// Heater2_Off시 출력 0
// ===== PID_v1 최소구성: =====
// - 출력(Output): 0..s_window_ms (ms)  → 윈도우 내 ON 시간
// - 입력(Input):  Heater_1_NTC_TEMP (°C)
// - 목표(Setpoint): c_tmp (°C)
// - 튜닝: Heter_PID_P, Heter_PID_I, Heter_PID_D 사용
// =====================================================

static double gSetpoint = 50.0;  // c_tmp를 복사해 씀
static double gInput = 25.0;     // Heater_1_NTC_TEMP


// ===== 윈도우/타이머 공통 =====
static esp_timer_handle_t s_window_timer = nullptr;  // 주기 타이머
static esp_timer_handle_t s_off_timer = nullptr;     // ON->OFF 원샷 타이머
static volatile uint32_t s_window_ms = 500;          // ★ 기본 반복시간(=윈도우) 1000ms

static inline uint64_t ms_to_us(uint32_t ms) {
  return (uint64_t)ms * 1000ULL;
}


// PID 객체 (DIRECT: 오류↑ → 출력↑)
static PID gPID(&gInput, &Heater1_output_value, &gSetpoint, 2, 5, 1, DIRECT);

// 마지막으로 적용한 튜닝 값 캐시(웹에서 바뀌면 동기화)
static double gLastKp = -1, gLastKi = -1, gLastKd = -1;

static inline void Heater1_PID_ApplyTuningsIfChanged() {

  // 입력/세트포인트 최신화
  gSetpoint = (double)c_tmp;           // 목표 온도(°C)
  gInput = (double)Heater_1_NTC_TEMP;  // 현재 온도(°C)

  const double Kp = Heter_PID_P;
  const double Ki = Heter_PID_I;
  const double Kd = Heter_PID_D;
  if (Kp != gLastKp || Ki != gLastKi || Kd != gLastKd) {
    gPID.SetTunings(Kp, Ki, Kd);
    gLastKp = Kp;
    gLastKi = Ki;
    gLastKd = Kd;
  }
}

// 초기화: GPIO/타이머 설정 내부에서 호출
static void Heater1_PID_Init() {
  gSetpoint = (double)c_tmp;
  gInput = (double)Heater_1_NTC_TEMP;

  Heater1_PID_ApplyTuningsIfChanged();

  // 출력 범위 = 0..윈도우(ms) → ON 시간(ms)
  gPID.SetOutputLimits(0, (double)s_window_ms);
  // 샘플타임 = 윈도우(ms) → 창 시작마다 1번 계산
  gPID.SetSampleTime((int)s_window_ms);

  // ★ 여기서 100%로 시작 (출력 단위=ms이므로 윈도우 길이 그대로)
  Heater1_output_value = (double)s_window_ms;

  // bumpless: 수동→자동 전환 시 위 Output을 적분 초기값으로 씀
  gPID.SetMode(MANUAL);
  gPID.SetMode(AUTOMATIC);
}

// 세트포인트가 바뀌면(웹 UI 등) 호출해 주면 좋음(선택)
void Heater1_OnSetpointChanged() {
  gSetpoint = (double)c_tmp;
  // PID_v1은 내부적으로 anti-windup이 간단해 리셋 불필요.
  // 필요시 gPID.SetMode(MANUAL); gPID.SetMode(AUTOMATIC);로 재동기화 가능.
}

// 매 윈도우 시작 시 호출: 현재 온도로 Compute() 1회
void Heater1_PID_Compute() {
  // 최신 튜닝 동기화
  Heater1_PID_ApplyTuningsIfChanged();


  // 출력 범위/샘플타임이 윈도우와 동일하도록 보장
  gPID.SetOutputLimits(0, (double)s_window_ms);
  // 샘플타임은 윈도우보다 20~50ms 작게
  int sample_ms = (int)s_window_ms - 20;  // ex) 480 ms
  gPID.SetSampleTime((int)sample_ms);

  // PID_v1 내부 타임(millis) 기준으로 샘플타임 경과 시 계산
  gPID.Compute();
}


// OFF 타이머 콜백: ON 구간 종료
static void off_timer_cb(void* arg) {
  (void)arg;
  gpio_set_level((gpio_num_t)Heater1_GPIO_SSR_PIN, 0);
}

// 윈도우 시작 콜백: PID_v1 계산 → Heater1_output_value(ms) 만큼 ON 예약
static void window_timer_cb(void* arg) {
  (void)arg;

  const bool allowed = (heater1_On && !heater1_Overheated());
  if (!allowed) {
    Heater1_ForceOff();  // 핀 LOW 보장
    // Output 0 보장(디스플레이/로깅 일관성)
    Heater1_output_value = 0.0;
    return;  // ★ PID.Compute() 건너뜀
  }

 // PID는 여기서만 1회
 Heater1_PID_Compute();   // ★ 래퍼 사용 (gInput/SP/튜닝/샘플타임 포함)

 Serial.printf("[H1] SP=%.1f PV=%.1f OUT=%.1f allowed=%d\n",
               gSetpoint, gInput, Heater1_output_value, (int)allowed);



               
  const uint64_t period_us = (uint64_t)s_window_ms * 1000ULL;
  uint64_t on_us = (uint64_t)lround(Heater1_output_value) * 1000ULL;
  if (on_us > period_us) on_us = period_us;

  if (on_us > 0) {
    gpio_set_level((gpio_num_t)Heater1_GPIO_SSR_PIN, 1);
    if (on_us < period_us) {
      if (s_off_timer) {
        esp_timer_stop(s_off_timer);
        esp_timer_start_once(s_off_timer, on_us);
      }
    } else {
      if (s_off_timer) esp_timer_stop(s_off_timer);
    }
  } else {
    gpio_set_level((gpio_num_t)Heater1_GPIO_SSR_PIN, 0);
    if (s_off_timer) esp_timer_stop(s_off_timer);
  }
}



// =====================================================
// Heater2 상태 관리 제어함수

void Heater1_GPIO_SSR_Setup() {
  pinMode(Heater1_GPIO_SSR_PIN, OUTPUT);
  digitalWrite(Heater1_GPIO_SSR_PIN, LOW);
  delay(50);

  // OFF 타이머
  if (!s_off_timer) {
    const esp_timer_create_args_t off_args = {
      .callback = &off_timer_cb,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,  // ISR 아님, task 문맥
      .name = "heater1_off"
    };
    esp_timer_create(&off_args, &s_off_timer);
  }

  // 윈도우 타이머 (주기적)
  if (!s_window_timer) {
    const esp_timer_create_args_t win_args = {
      .callback = &window_timer_cb,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "heater1_window"
    };
    esp_timer_create(&win_args, &s_window_timer);
  }

  // PID_v1 초기화
  Heater1_PID_Init();

  // 고정 주기로 시작
  esp_timer_start_periodic(s_window_timer, (uint64_t)s_window_ms * 1000ULL);
}









// 안전하게 핀 OFF + 상태 정리
static void Heater1_ForceOff() {
  gpio_set_level((gpio_num_t)Heater1_GPIO_SSR_PIN, 0);
  if (s_off_timer) esp_timer_stop(s_off_timer);
}
void Heater1_GPIO_ON() {
  heater1_On = true;
  // // 선택지 A: 첫 창 100%로 예열 시작
  // Heater1_output_value = (double)s_window_ms;  // 100% 프리
  // // bumpless 전환: 현재 Output을 ITerm 초기값으로 씀
  // gPID.SetMode(MANUAL);
  gPID.SetMode(AUTOMATIC);
}
void Heater1_GPIO_OFF() {
  heater1_On = false;
  gPID.SetMode(MANUAL);  // 적분/내부상태 정지
  Heater1_ForceOff();    // 즉시 핀 LOW, 오프 타이머 정지
}





// =====================================================
// Heater2
// =====================================================
void Heater2_GPIO_Setup() {
  pinMode(Heater2_GPIO_RELAY_PIN, OUTPUT);
  digitalWrite(Heater2_GPIO_RELAY_PIN, LOW);
  delay(100);
}

void Heater2_GPIO_ON() {
  heater2_On = true;
}
void Heater2_GPIO_OFF() {
  heater2_On = false;
}
void Heater2_GPIO_Write() {
  if (!heater2_On || heater2_Overheated()) digitalWrite(Heater2_GPIO_RELAY_PIN, LOW);
  else digitalWrite(Heater2_GPIO_RELAY_PIN, HIGH);
}