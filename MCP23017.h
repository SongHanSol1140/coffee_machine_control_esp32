// MCP23017.h
#ifndef MCP23017_H
#define MCP23017_H

#include <Arduino.h>

// 포트/핀 방향 프리셋


// 초기화/유틸
void MCP23017_Expander_Init(int sda, int scl, uint8_t address);
void expanderWriteForDoc(int pin, int level);
bool expanderReadForDoc(int pin);
void expanderPaWrite(int pin, int level);
void expanderPbWrite(int pin, int level);
void expanderPaRead(int pin);
void expanderPbRead(int pin);

#endif // MCP23017_H