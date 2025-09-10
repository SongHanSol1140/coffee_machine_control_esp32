// Heater2.h
#ifndef HEATER_H
#define HEATER_H

extern void Heater1_GPIO_Setup();
extern void Heater2_GPIO_Setup();

extern void Heater1_GPIO_ON();
extern void Heater1_GPIO_OFF();
extern void Heater1_PID_Compute();

// ★ 추가: 세트포인트 변경 시 적분 리셋용
void Heater1_OnSetpointChanged();

extern void Heater2_GPIO_ON();
extern void Heater2_GPIO_OFF();
extern void Heater2_GPIO_Write();




#endif // HEATER_H