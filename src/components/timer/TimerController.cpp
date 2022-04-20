//
// Created by florian on 16.05.21.
//

#include "components/timer/TimerController.h"
#include "systemtask/SystemTask.h"
#include "task.h"

using namespace Pinetime::Controllers;

namespace {
  void TimerEnd(TimerHandle_t xTimer) {
    auto controller = static_cast<Pinetime::Controllers::TimerController*>(pvTimerGetTimerID(xTimer));
    controller->OnTimerEnd();
  }
}

void TimerController::Init(System::SystemTask* systemTask) {
  this->systemTask = systemTask;
  timerAppTimer = xTimerCreate("timerAppTm", 1, pdFALSE, this, TimerEnd);
}

void TimerController::StartTimer(uint32_t duration) {
  xTimerStop(timerAppTimer, 0);
  auto currentTicks = xTaskGetTickCount();
  TickType_t durationTicks = APP_TIMER_TICKS(duration);
  xTimerChangePeriod(timerAppTimer, durationTicks, 0);
  xTimerStart(timerAppTimer, 0);
  endTicks = currentTicks + durationTicks;
  timerRunning = true;
}

uint32_t TimerController::GetTimeRemaining() {
  if (!timerRunning) {
    return 0;
  }
  auto currentTicks = xTaskGetTickCount();

  TickType_t deltaTicks = 0;
  if (currentTicks > endTicks) {
    deltaTicks = 0xffffffff - currentTicks;
    deltaTicks += (endTicks + 1);
  } else {
    deltaTicks = endTicks - currentTicks;
  }

  return (static_cast<TickType_t>(deltaTicks) / static_cast<TickType_t>(configTICK_RATE_HZ)) * 1000;
}

void TimerController::StopTimer() {
  xTimerStop(timerAppTimer, 0);
  timerRunning = false;
}

bool TimerController::IsRunning() {
  return timerRunning;
}
void TimerController::OnTimerEnd() {
  timerRunning = false;
  if (systemTask != nullptr)
    systemTask->PushMessage(System::Messages::OnTimerDone);
}
