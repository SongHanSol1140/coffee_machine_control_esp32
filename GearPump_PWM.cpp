// GearPump_PWM.cpp
#include <Arduino.h>
#include "variables.h"
#include "GearPump_PWM.h"

static constexpr int GEAR_PUMP_PWM_FREQ_HZ  = 2000; // Hz
static constexpr int GEAR_PUMP_PWM_RES_BITS = 8;   // 10-bit -> duty 0~1023 / 8-bit -> duty 0~255
static constexpr int GEAR_PUMP_PWM_MAX_DUTY = (1 << GEAR_PUMP_PWM_RES_BITS) - 1;


void GearPump_PWM_Output_Setup(){
  ledcAttach(GearPump_PWM_inputPIN, GEAR_PUMP_PWM_FREQ_HZ, GEAR_PUMP_PWM_RES_BITS);
  ledcWrite(GearPump_PWM_inputPIN, 0);
  delay(100);
}

void GearPump_PWM_Output_Write(){
  int percent = GearPump_PWM_output_percent; // variables.cpp의 전역
  if (percent < 0)   percent = 0;
  if (percent > 100) percent = 100;

  int duty = (GEAR_PUMP_PWM_MAX_DUTY * percent) / 100;
  ledcWrite(GearPump_PWM_inputPIN, duty);
}