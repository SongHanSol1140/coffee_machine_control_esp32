// variables.cpp
#include "variables.h"


// ESP32 커피 제조 기능 On/Off 변수
bool isRunning = false; // 커피머신 제조시작
bool isWorking = false; // 제조 중인지 확인
bool emergencyStop = false;
bool isHot = true;
bool isCold = false;
// ================================================
// CT ADC 입력핀 (암페어로 환산하여 emergencyAmpere값 초과시 emergencyStop)
float emergencyAmpere = 5;
float currentAmpere = 0;
const int CT_emergencyAmpere_check_PIN = 27;
// ================================================
// 청소 시간
int cleaning_time = 0;
int cleaning_time_all = 0;
// 공기 흡입
int air_inhale_start_after_waiting_time = 0; // 공기 흡입 시작 후 대기 시간
int air_inhale_time = 0; // 공기 흡입 작동 시간
int air_inhale_on_time = 0; // 공기 흡입 ON 시간
int air_inhale_off_time = 0; // 공기 흡입 OFF 시간
// 혼합 시간
int create_shake_time = 0;
// 드레인 시간
int drain_time = 0;
// ================================================
// MCP23017 GPIO EXPANDER
const int MCP23017_SDA = 21;
const int MCP23017_SCL = 22;
// ================================================
// 유량계 YF-S402B
// 음료 출구 측정 핀
const int YF_S402B_PIN_OUTPUT_PIN = 36;
// 혼합탱크 들어가는량 측정 핀
const int YF_S402B_PIN_INPUT_PIN = 39;
// 총 측정 유량
float YF_S402B_outputFlow = 0;
float YF_S402B_inputFlow = 0;
// ================================================
// NTC 온도 센서 측정 핀
const int Heater_1_NTC_PIN = 34;
const int Heater_2_NTC_PIN = 35;
// NTC 온도 센서 측정 값
float Heater_1_NTC_TEMP = 0;
float Heater_2_NTC_TEMP = 0;
// ================================================
// 히터 제어
const int Heater_1_GPIO_PIN = 25;
const int Heater_2_PWM_PIN = 33;
int Heater_2_PWM_output_value = 0;
double Heter_PID_P = 0.5;
double Heter_PID_I = 0.05;
double Heter_PID_D = 0.02;
// ================================================
// 기어모터 출력핀 (소프트 스타트 제어)
const int GearPump_PWM_inputPIN = 32;
// ================================================

// ================================================

// ================================================
// 커피 제조 값 (제조시 대입할 값, 웹페이지에는 보이지 않음)
int create_espresso_ml = 0;
int create_water_ml = 0;
int create_milk_ml = 0;
int create_temperature = 0;
// 에스프레소 설정 값
int create_espresso_ml_setting = 0;
int create_temperature_setting = 0;
// 아메리카노 설정값
int create_americano_espresso_ml_setting = 0;
int create_americano_water_ml_setting = 0;
int create_americano_temperature_setting = 0;
// 카페라떼 설정값
int create_cafelatte_espresso_ml_setting = 0;
int create_cafelatte_milk_ml_setting = 0;
int create_cafelatte_temperature_setting = 0;
// 청소 시간 설정값
int cleaning_time_setting = 0;
int cleaning_time_all_setting = 0;
// 공기 흡입 설정값
int air_inhale_start_after_waiting_time_setting = 0;
int air_inhale_time_setting = 0;
int air_inhale_on_time_setting = 0;
int air_inhale_off_time_setting = 0;
// 혼합 시간 설정값
int create_shake_time_setting = 0;
// 기어 펌프 출력 퍼센트
int GearPump_PWM_output_percent = 0;
// 드레인 시간 설정값
int drain_time_setting = 0;
// 현재 유량이 목표 유량의 limit_percent 이상일시 히터 2 off
extern int create_heater_2_off_flow_limit_percent;