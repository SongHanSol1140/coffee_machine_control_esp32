// variables.h
#ifndef VARIABLES_H
#define VARIABLES_H

// ESP32 커피 제조 기능 On/Off 변수
extern bool isRunning; // 커피머신 제조 시작
extern bool isWorking; // 제조 중인지 확인
extern bool emergencyStop;
extern bool isHot;
extern bool isCold;
// ================================================
// CT ADC 입력핀 (암페어로 환산하여 emergencyAmpere값 초과시 emergencyStop)
extern float emergencyAmpere;
extern float currentAmpere;
extern const int CT_emergencyAmpere_check_PIN;
// ================================================
// 청소 시간
extern int cleaning_time;
extern int cleaning_time_all;
// 공기 흡입
extern int air_inhale_start_after_waiting_time; // 공기 흡입 시작 후 대기 시간
extern int air_inhale_time; // 공기 흡입 작동 시간
extern int air_inhale_on_time; // 공기 흡입 온 시간
extern int air_inhale_off_time; // 공기 흡입 끄는 시간
// 혼합 시간
extern int create_shake_time;
// 드레인 시간
extern int drain_time;
// ================================================
// MCP23017 GPIO EXPANDER
extern const int MCP23017_SDA;
extern const int MCP23017_SCL;
// ================================================
// 유량계 YF-S402B
// 음료 출구 측정 핀
extern const int YF_S402B_PIN_OUTPUT_PIN;
// 혼합탱크 들어가는량 측정 핀
extern const int YF_S402B_PIN_INPUT_PIN;
// 총 측정 유량
extern float YF_S402B_outputFlow;
extern float YF_S402B_inputFlow;
// ================================================
// NTC 온도 센서 측정 핀
extern const int Heater_1_NTC_PIN;
extern const int Heater_2_NTC_PIN;
// NTC 온도 센서 측정 값
extern float Heater_1_NTC_TEMP;
extern float Heater_2_NTC_TEMP;
// ================================================
// 히터 제어
extern const int Heater_1_GPIO_PIN;
extern const int Heater_2_PWM_PIN;
extern int Heater_2_PWM_output_value;
extern double Heter_PID_P;
extern double Heter_PID_I;
extern double Heter_PID_D;
// ================================================
// 기어모터 출력핀 (소프트 스타트 제어)
extern const int GearPump_PWM_inputPIN;
// // ================================================

// ================================================

// // ================================================
// 커피 제조 값 (제조시 대입할 값, 웹페이지에는 보이지 않음)
extern int create_espresso_ml;
extern int create_water_ml;
extern int create_milk_ml;
extern int create_temperature;
// 에스프레소 설정값
extern int create_espresso_ml_setting;
extern int create_water_ml_setting;
// 아메리카노 설정값
extern int create_americano_espresso_ml_setting;
extern int create_americano_water_ml_setting;
extern int create_americano_temperature_setting;
// 카페라떼 설정값
extern int create_cafelatte_espresso_ml_setting;
extern int create_cafelatte_milk_ml_setting;
extern int create_cafelatte_temperature_setting;
// 청소 시간 설정값
extern int cleaning_time_setting;
extern int cleaning_time_all_setting;
// 공기 흡입 설정값
extern int air_inhale_start_after_waiting_time_setting;
extern int air_inhale_time_setting;
extern int air_inhale_on_time_setting;
extern int air_inhale_off_time_setting;
// 혼합 시간 설정값
extern int create_shake_time_setting;
// 기어 펌프 출력 퍼센트
extern int GearPump_PWM_output_percent;
// 드레인 시간 설정값
extern int drain_time_setting;
// 현재 유량이 목표 유량의 limit_percent 이상일시 히터 2 off
extern int create_heater_2_off_flow_limit_percent;
#endif // VARIABLES_H