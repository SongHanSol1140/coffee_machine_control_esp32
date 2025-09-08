#include <Arduino.h> // Interrupt 사용을 위한 라이브러리
// YF_S402_FlowMeter.cpp
#include "YF_S402B_FlowMeter.h"
#include "variables.h"
// 유량 계산용 변수
// 유량계 펄스 카운트
int numberOfPulseOutput = 0;
int numberOfPulseInput = 0;
// FlowRate 카운트
float flowRateOutput = 0;
float flowRateInput = 0;
// 시간측정용 변수
unsigned long outputLastTime = 0;
unsigned long inputLastTime = 0;
// 업데이트 최소 주기(ms). 정확도는 elapsed 보정하므로 값은 크게 상관없음.
const unsigned long MIN_UPDATE_INTERVAL_MS = 200;

void outputRpmCheck()
{
	numberOfPulseOutput++;
}
void inputRpmCheck()
{
	numberOfPulseInput++;
}
void FlowMeter_Setup()
{
	pinMode(YF_S402B_PIN_OUTPUT_PIN, INPUT);
	pinMode(YF_S402B_PIN_INPUT_PIN, INPUT);

	attachInterrupt(digitalPinToInterrupt(YF_S402B_PIN_OUTPUT_PIN), outputRpmCheck, FALLING);
	attachInterrupt(digitalPinToInterrupt(YF_S402B_PIN_INPUT_PIN), inputRpmCheck, FALLING);

	Serial.println("Flow Meter Setup Complete");
	delay(1000);
}

void flowMeter_Output_Read(){
	unsigned long now = millis();
	unsigned long elapsed = now - outputLastTime;
	if (elapsed < MIN_UPDATE_INTERVAL_MS) return;

	outputLastTime = now;

	noInterrupts();
	uint32_t pulses = numberOfPulseOutput;
	numberOfPulseOutput = 0;
	interrupts();

	float seconds = elapsed / 1000.0f;
	if (seconds <= 0.0f) return;

	// 순간 유량(mL/s)
	float flowLMin = (pulses / seconds) / 38.0f;
	flowRateOutput = (flowLMin * 1000.0f) / 60.0f;
    // 유량 누적
	YF_S402B_outputFlow += pulses * (1000.0f / (38.0f * 60.0f)); // 누적 mL
}

void flowMeter_Input_Read() {
	unsigned long now = millis();
	unsigned long elapsed = now - inputLastTime;
	if (elapsed < MIN_UPDATE_INTERVAL_MS) return;
  
	inputLastTime = now;
  
	noInterrupts();
	uint32_t pulses = numberOfPulseInput;
	numberOfPulseInput = 0;
	interrupts();
  
	float seconds = elapsed / 1000.0f;
	if (seconds <= 0.0f) return;
  
	float flowLMin = (pulses / seconds) / 38.0f;
	flowRateInput = (flowLMin * 1000.0f) / 60.0f;

	YF_S402B_inputFlow += pulses * (1000.0f / (38.0f * 60.0f)); // 누적 mL
	
}