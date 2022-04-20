#pragma once

#include <FreeRTOS.h>
#include <timers.h>
#include <cstdint>
#include "app_timer.h"
#include "portmacro_cmsis.h"

namespace Pinetime {
  namespace System {
    class SystemTask;
  }
  namespace Controllers {

    class TimerController {
    public:
      TimerController() = default;

      void StartTimer(uint32_t duration);
      void StopTimer();

      uint32_t GetTimeRemaining();

      bool IsRunning();

      void OnTimerEnd();

    protected:
      friend class Pinetime::System::SystemTask;
      void Init(System::SystemTask* systemTask);

    private:
      System::SystemTask* systemTask = nullptr;
      TimerHandle_t timerAppTimer;
      TickType_t endTicks;
      bool timerRunning = false;
    };
  }
}