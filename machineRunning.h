// machineRunning.h
#ifndef MACHINE_RUNNING_H
#define MACHINE_RUNNING_H

extern void stopAllOutputsAndResetFlags();
void createEspresso();
void createAmericano();
void createCafeLatte();
void createCleaning();
// FreeRTOS 태스크 함수 정의
void espressoTask(void *parameter);

// FreeRTOS 태스크 함수 정의 (아메리카노)
void americanoTask(void *parameter);

// FreeRTOS 태스크 함수 정의 (카페라떼)
void cafelatteTask(void *parameter);

// FreeRTOS 태스크 함수 정의 (청소)
void cleaningTask(void *parameter);

#endif // MACHINE_RUNNING_H