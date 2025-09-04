// variables.cpp
#include "variables.h"

// ESP32 커피 제조 기능 On/Off 변수
bool isRunning = false;
bool isWorking = false;
bool emergencyStop = false;
bool isHot = true;
bool isCold = false;

// CT ADC 입력핀
float emergencyAmpere = 5;
float currentAmpere = 0;
const int CT_emergencyAmpere_check_PIN = 27;

// 청소 시간
int cleaning_time = 0;
int cleaning_time_all = 0;

// 공기 흡입
int air_inhale_start_after_waiting_time = 0;
int air_inhale_time = 0;
int air_inhale_on_time = 0;
int air_inhale_off_time = 0;

// 혼합 시간
int create_shake_time = 0;
int drain_time = 0;

// MCP23017 GPIO EXPANDER
const int MCP23017_SDA = 21;
const int MCP23017_SCL = 22;

// 유량계 YF‑S402B
const int YF_S402B_PIN_OUTPUT_PIN = 36;
const int YF_S402B_PIN_INPUT_PIN = 39;
float YF_S402B_outputFlow = 0;
float YF_S402B_inputFlow = 0;

// NTC 온도 센서
const int Heater_1_NTC_PIN = 34;
const int Heater_2_NTC_PIN = 35;
float Heater_1_NTC_TEMP = 0;
float Heater_2_NTC_TEMP = 0;

// 히터 제어
const int Heater_1_GPIO_PIN = 25;
const int Heater_2_PWM_PIN = 33;
int Heater_2_PWM_output_value = 0;
double Heter_PID_P = 0.5;
double Heter_PID_I = 0.05;
double Heter_PID_D = 0.02;

// 기어모터 출력핀
const int GearPump_PWM_inputPIN = 32;

// 커피 제조 값
int create_espresso_ml = 0;
int create_water_ml = 0;
int create_milk_ml = 0;
int create_temperature = 0;

// 설정값
int create_espresso_ml_setting = 0;
int create_water_ml_setting = 0;
int create_americano_espresso_ml_setting = 0;
int create_americano_water_ml_setting = 0;
int create_americano_temperature_setting = 0;
int create_cafelatte_espresso_ml_setting = 0;
int create_cafelatte_milk_ml_setting = 0;
int create_cafelatte_temperature_setting = 0;
int cleaning_time_setting = 0;
int cleaning_time_all_setting = 0;
int air_inhale_start_after_waiting_time_setting = 0;
int air_inhale_time_setting = 0;
int air_inhale_on_time_setting = 0;
int air_inhale_off_time_setting = 0;
int create_shake_time_setting = 0;
int GearPump_PWM_output_percent = 0;
int drain_time_setting = 0;
int create_heater_2_off_flow_limit_percent = 0;
