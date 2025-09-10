// variables.cpp
#include "variables.h"

// ESP32 커피 제조 기능 On/Off 변수
volatile bool isRunning = false;
volatile bool isWorking = false;
volatile bool emergencyStop = false;
bool isHot = true;
bool isCold = false;

// CT ADC 입력핀
float emergencyA = 5;
volatile float currentAmpere = 0;
const int CT_emergencyAmpere_check_PIN = 36; // VP
int ctAdcZero = 3025;
float ctAnalogValue = 0;

// MCP23017
const int MCP23017_SDA = 21;
const int MCP23017_SCL = 22;

// 유량계
const int YF_S402B_PIN_OUTPUT_PIN = 18;
const int YF_S402B_PIN_INPUT_PIN  = 19;
volatile float YF_S402B_outputFlow = 0;
volatile float YF_S402B_inputFlow  = 0;

// NTC 온도센서
const int Heater_1_NTC_PIN = 35;
const int Heater_2_NTC_PIN = 34;
volatile float Heater_1_NTC_TEMP = 0;
volatile float Heater_2_NTC_TEMP = 0;

// 히터 위험 온도 기본값 초기화 (℃)
// 사용자가 PID 탭에서 조절할 수 있으며, 시스템이 재부팅되어도 NVS에서 복원됩니다.
// 커피 머신은 물을 끓이기 위해 대략 90~100℃ 사이의 온도가 필요하지만,
// 기본값을 100으로 설정하여 안전 영역을 확보했습니다.
float h1_emer_tmp = 100.0f;
float h2_emer_tmp2 = 100.0f;

// 히터 제어
const int Heater1_GPIO_SSR_PIN = 33;
const int Heater2_GPIO_RELAY_PIN  = 25;
double Heater1_output_value = 0.0;
double Heter_PID_P = 0.5;
double Heter_PID_I = 0.05;
double Heter_PID_D = 0.02;

// 기어모터
const int GearPump_PWM_outputPIN = 32;

// 커피 제조 값 (실제 제조 시 사용)
int c_esspresso_ml = 0;
int c_water_ml     = 0;
int c_milk_ml      = 0;
int c_tmp          = 30;

// 설정값
int e_ml_set     = 0;
int e_tmp_set = 0;
int a_e_ml_set   = 0;
int a_w_ml_set   = 0;
int a_tmp_set    = 0;
int c_e_ml_set   = 0;
int c_m_ml_set   = 0;
int c_tmp_set    = 0;
int clean_time   = 0;
int clean_time_all = 0;
int inhale_w_time  = 0; // 공기 흡입 시작 대기시간
int inhale_time    = 0; // 공기 흡입 시간
int inhale_on_time = 0; // 공기 흡입 ON 시간
int inhale_off_time= 0; // 공기 흡입 OFF 시간
int shake_time    = 0;
int pump_out_per  = 50; // 기어펌프 출력 %(기어펌프 듀티사이클은 고정)
int drain_time    = 0; // 드레인 시간
int h2_limit_per  = 50; // 히터2 유량 OFF 한계%


// GPIO 상태 출력 변수
// ESP32 GPIO 상태
bool ESP32_GPIO25 = false; // 히터 #1

// MCP23017 GPIO Expander
// PA
bool expanderGPIO1 = false;
bool expanderGPIO2 = false;
bool expanderGPIO3 = false;
bool expanderGPIO4 = false;
bool expanderGPIO5 = false;
bool expanderGPIO6 = false;
bool expanderGPIO7 = false;
bool expanderGPIO8 = false;
// PB
bool expanderGPIO9 = false;
bool expanderGPIO10 = false;

// PWM 상태 플래그 초기화
// 기어펌프와 히터2 PWM 상태를 추적합니다.
bool gearPumpOn = false;
volatile bool heater1_On = false;
volatile bool heater2_On = false;
