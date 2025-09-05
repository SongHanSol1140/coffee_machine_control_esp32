// MCP23017.cpp
#include "variables.h"
#include "MCP23017.h"
#include <Wire.h>
#include <Adafruit_MCP23X17.h>


static Adafruit_MCP23X17 mcp1; // 0x20
// static Adafruit_MCP23X17 mcp2; // 0x21
// static Adafruit_MCP23X17 mcp3; // 0x22
// static Adafruit_MCP23X17 mcp4; // 0x23
// static Adafruit_MCP23X17 mcp5; // 0x24
// static Adafruit_MCP23X17 mcp6; // 0x25
// static Adafruit_MCP23X17 mcp7; // 0x26
// static Adafruit_MCP23X17 mcp8; // 0x27

void MCP23017_Expander_Init(int sda, int scl, uint8_t address){
	Wire.begin(sda, scl);
	if (!mcp1.begin_I2C(address)){
		Serial.println("MCP23017 1 not found");
		while (100);
	};
	delay(100);
	// mcp1의 PORTA 출력 / PORTB 입력 설정
	// PORTA : 0~7 출력설정
	// mcp1.pinMode(i, OUTPUT);
	// PORTB : 8~15 입력설정
	// mcp1.pinMode(i + 8, INPUT_PULLUP);
	for (int i = 0; i < 8; i++){
		// PORTA / PORTB 출력 설정
		mcp1.pinMode(i, OUTPUT);
		mcp1.pinMode(i + 8, OUTPUT);
	}
	delay(100);
	// PORTA를 모두 HIGH로 설정
	mcp1.writeGPIOA(0x00);
	// PORTB를 모두 LOW 설정
	mcp1.writeGPIOB(0x00);

	// 핀상태 검사
	Serial.print("PA0 핀 상태: ");
	Serial.println(mcp1.digitalRead(0));
	Serial.print("PB0 핀 상태: ");
	Serial.println(mcp1.digitalRead(8));


	Serial.println("MCP23017 Expander Setup Complete");
	delay(100);
	// MCP23017 Expander Setup Complete
}
// 원래는 0~15지만 
// 설계도를 쉽게 참조하기위해 1~16으로 변경
void expanderWriteForDoc(int pin, int level){
	mcp1.digitalWrite(pin-1, level);
}
bool expanderReadForDoc(int pin){
    // 1~10 핀 번호를 0~9 인덱스로 변환하여 읽은 값을 반환합니다.
    return mcp1.digitalRead(pin - 1);
}

// ===============================================

void expanderPaWrite(int pin, int level){
	mcp1.digitalWrite(pin -1, level);
}

void expanderPbWrite(int pin, int level){
	mcp1.digitalWrite(pin + 8, level);
}
void expanderPaRead(int pin){
	Serial.println(mcp1.digitalRead(pin));
}
void expanderPbRead(int pin){
	Serial.println(mcp1.digitalRead(pin + 8));

}