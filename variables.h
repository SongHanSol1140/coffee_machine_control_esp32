#ifndef VARIABLES_H
#define VARIABLES_H

// ESP32 커피 제조 기능 On/Off 변수
extern bool isRunning;
extern bool isWorking;
extern bool emergencyStop;
extern bool isHot;
extern bool isCold;

// CT ADC 입력핀 (암페어로 환산하여 emergencyA값 초과 시 emergencyStop)
extern float emergencyA;
extern float currentAmpere;
extern const int CT_emergencyAmpere_check_PIN;

// MCP23017 GPIO EXPANDER
extern const int MCP23017_SDA;
extern const int MCP23017_SCL;

// 유량계 YF‑S402B
extern const int YF_S402B_PIN_OUTPUT_PIN;
extern const int YF_S402B_PIN_INPUT_PIN;
extern float YF_S402B_outputFlow;
extern float YF_S402B_inputFlow;

// NTC 온도 센서
extern const int Heater_1_NTC_PIN;
extern const int Heater_2_NTC_PIN;
extern float Heater_1_NTC_TEMP;
extern float Heater_2_NTC_TEMP;

// 히터 제어
extern const int Heater_1_GPIO_PIN;
extern const int Heater_2_PWM_PIN;
extern int Heater_2_PWM_output_value;
extern double Heter_PID_P;
extern double Heter_PID_I;
extern double Heter_PID_D;

// 기어모터 출력핀 (소프트 스타트)
extern const int GearPump_PWM_inputPIN;

// 커피 제조 값 (제조 시 대입)
extern int c_esspresso_ml;
extern int c_water_ml;
extern int c_milk_ml;
extern int c_tmp;

// 설정값 (15자 이하 키)
extern int e_ml_set;      // 에스프레소 에스프레소 양
extern int water_ml_set;  // 에스프레소 물 양
extern int a_e_ml_set;    // 아메리카노 에스프레소 양
extern int a_w_ml_set;    // 아메리카노 물 양
extern int a_tmp_set;     // 아메리카노 온도
extern int c_e_ml_set;    // 카페라떼 에스프레소 양
extern int c_m_ml_set;    // 카페라떼 우유 양
extern int c_tmp_set;     // 카페라떼 온도
extern int clean_time;    // 청소 시간
extern int clean_time_all;// 전체 청소 시간
extern int inhale_w_time; // 공기흡입 시작 대기시간
extern int inhale_time;   // 공기흡입 시간
extern int inhale_on_time;// 공기흡입 ON 시간
extern int inhale_off_time;// 공기흡입 OFF 시간
extern int shake_time;    // 혼합 시간
extern int pump_out_per;  // 기어펌프 출력 %
extern int drain_time;    // 드레인 시간
extern int h2_limit_per;  // 히터2 OFF 흐름 한계 %

#endif // VARIABLES_H
