// .ino 
#include "variables.h"
#include "wifiSetup.h"
#include "MCP23017.h"
#include "YF_S402B_FlowMeter.h"
#include "NTC_TempertureSensor.h"
#include "Heater.h"
#include "CT_Emergency_Check.h"
#include "GearPump_PWM.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "machineRunning.h"

void setup() {
  Serial.begin(115200);

  loadSettings();

  MCP23017_Expander_Init(MCP23017_SDA, MCP23017_SCL, 0x20);
  FlowMeter_Setup();
  NTC_Temperture_Setup();
  Heater1_GPIO_Setup();
  Heater2_GPIO_Setup();
  GearPump_PWM_Setup();

  // 2) WiFi / WebServer 시작
  wifiSetup();
  startWebServer();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiSetup();
  }
  delay(2);
  flowMeter_Output_Read();
  flowMeter_Input_Read();
  delay(2);
  heater_1_NTC_Temperture_Read();
  heater_2_NTC_Temperture_Read();
  delay(2);
  Heater2_GPIO_Write();
  delay(2);
  CT_Emergency_Check();
  delay(2);
  
}