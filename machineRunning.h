// machineRunning.h
#ifndef MACHINE_RUNNING_H
#define MACHINE_RUNNING_H

// 에스프레소 제조를 시작하는 함수. html의 "제조 시작" 버튼 클릭 시 호출된다.
// isRunning이 true이고 현재 작업 중이 아닐 때만 동작하며, 제작 중에는 isWorking 플래그를 사용해 중복 호출을 방지한다.
// 제조 과정에서 currentAmpere가 emergencyA를 초과하거나 emergencyStop이 true로 변하면 즉시 중지한다.
// 제조 완료 후 isWorking을 false로 되돌린다.
void createEspresso();

// FreeRTOS 태스크 함수 정의
void espressoTask(void *parameter);

#endif // MACHINE_RUNNING_H