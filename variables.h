// variables.h
#ifndef VARIABLES_H
#define VARIABLES_H

// ESP32 커피 제조 기능 On/Off 변수
volatile extern bool isRunning;
volatile extern bool isWorking;
volatile extern bool emergencyStop;
extern bool isHot;
extern bool isCold;

// CT ADC 입력핀 (암페어로 환산하여 emergencyA값 초과 시 emergencyStop)
extern float emergencyA;
extern float currentAmpere;
extern const int CT_emergencyAmpere_check_PIN;
extern int ctAdcZero;
extern float ctAnalogValue;

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

// 각 히터의 위험 온도 설정값 (℃)
// 히터 #1(NTC1)과 히터 #2(NTC2)가 이 온도 이상으로 올라가면
// 히터 출력이 강제로 차단되며, 온도가 일정값 이하로 떨어지면 자동으로 다시 켜집니다.
// 기본값은 100℃로 설정되어 있으며 웹 UI를 통해 변경 가능하며 NVS에 저장됩니다.
extern float h1_emer_tmp;
extern float h2_emer_tmp2;

// 히터 제어
extern const int Heater_1_GPIO_PIN;
extern const int Heater_2_PWM_PIN;
extern int Heater_2_PWM_output_value;
extern double Heter_PID_P;
extern double Heter_PID_I;
extern double Heter_PID_D;

// 기어모터 출력핀 (소프트 스타트)
extern const int GearPump_PWM_outputPIN;

// 커피 제조 값 (제조 시 대입)
extern int c_esspresso_ml;
extern int c_water_ml;
extern int c_milk_ml;
extern int c_tmp;

// 설정값 (15자 이하 키)
extern int e_ml_set;      // 에스프레소 에스프레소 양
extern int e_tmp_set;  // 에스프레소 물 양 water_ml_set
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



// GPIO 상태 출력 변수
// ESP32 GPIO 상태
extern bool ESP32_GPIO25; // 히터 #1

// MCP23017 GPIO Expander
// PA
extern bool expanderGPIO1; // Solenoid Valve #1			
extern bool expanderGPIO2; // Solenoid Valve #2
extern bool expanderGPIO3; // Solenoid Valve #3
extern bool expanderGPIO4; // Solenoid Valve #4
extern bool expanderGPIO5; // Solenoid Valve #5
extern bool expanderGPIO6; // 3Way Valve #1
extern bool expanderGPIO7; // 3Way Valve #2
extern bool expanderGPIO8; // 3Way Valve #3
// PB
extern bool expanderGPIO9; // Pump #1
extern bool expanderGPIO10; // 릴레이 #2

// === PWM 상태 플래그 ===
// 웹페이지에서 PWM 출력을 ON/OFF 할 수 있도록 상태를 저장합니다.
// GearPump_PWM_ON() 호출 시 true, GearPump_PWM_OFF() 호출 시 false로 설정됩니다.
extern bool gearPumpOn;
// Heater2 PWM(SSR) 출력 상태. Heater2_PWM_Write() 호출 시 true, Heater2_PWM_ForceOff() 호출 시 false로 설정됩니다.
extern bool heater1_On;
extern bool heater2_On;

// === 수동 히터 제어 플래그 ===
// gpio_state_view 에서 히터 출력을 테스트하기 위해 사용합니다.
// manualHeater1 이 true 이면 히터1(릴레이)을 수동으로 켜려는 상태이고,
// manualHeater2 가 true 이면 히터2(PWM SSR)를 수동으로 켜려는 상태입니다.
// createEspresso/createAmericano/createCafeLatte 와 같은 제조 로직이 실행 중일 때는
// manual 플래그는 무시되고 제조 로직 내부에서 히터를 제어합니다.

#endif // VARIABLES_H
