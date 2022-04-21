#pragma once

#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "systemtask/SystemTask.h"
#include "displayapp/LittleVgl.h"

#include "components/timer/TimerController.h"

namespace Pinetime::Applications::Screens {
  class Timer : public Screen {
  public:
    enum class Modes { Normal, Done };

    Timer(DisplayApp* app, Controllers::TimerController& timerController);
    ~Timer() override;
    void Refresh() override;
    void RefreshRunning();
    inline void SetDone() {
      lv_label_set_text_static(time, "00:00");
      RefreshRunning();
      secondsToSet = 0;
      minutesToSet = 0;
      CreateButtons();
    }
  private:
    static void btnEventHandler(lv_obj_t* obj, lv_event_t event);
    inline void OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
      if (event == LV_EVENT_CLICKED) {
        if (obj == btnPlayPause) {
          if (timerController.IsRunning()) {
            uint32_t seconds = timerController.GetTimeRemaining() / 1000;
            minutesToSet = seconds / 60;
            secondsToSet = seconds % 60;
            timerController.StopTimer();
            CreateButtons();
            RefreshRunning();
          } else if (secondsToSet + minutesToSet > 0) {
            timerController.StartTimer((secondsToSet + minutesToSet * 60) * 1000);
            RefreshRunning();
            DeleteButtons();
          }
        } else {
          if (!timerController.IsRunning()) {
            if (obj == btnMinutesUp) {
              if (minutesToSet >= 59) {
                minutesToSet = 0;
              } else {
                minutesToSet++;
              }
              lv_label_set_text_fmt(time, "%02d:%02d", minutesToSet, secondsToSet);

            } else if (obj == btnMinutesDown) {
              if (minutesToSet == 0) {
                minutesToSet = 59;
              } else {
                minutesToSet--;
              }
              lv_label_set_text_fmt(time, "%02d:%02d", minutesToSet, secondsToSet);

            } else if (obj == btnSecondsUp) {
              if (secondsToSet >= 59) {
                secondsToSet = 0;
              } else {
                secondsToSet++;
              }
              lv_label_set_text_fmt(time, "%02d:%02d", minutesToSet, secondsToSet);

            } else if (obj == btnSecondsDown) {
              if (secondsToSet == 0) {
                secondsToSet = 59;
              } else {
                secondsToSet--;
              }
              lv_label_set_text_fmt(time, "%02d:%02d", minutesToSet, secondsToSet);
            }
          }
        }
      }
    }
    void CreateButtons();
    inline void DeleteButtons() {
      lv_obj_del(btnSecondsDown);
      btnSecondsDown = nullptr;
      lv_obj_del(btnSecondsUp);
      btnSecondsUp = nullptr;
      lv_obj_del(btnMinutesDown);
      btnMinutesDown = nullptr;
      lv_obj_del(btnMinutesUp);
      btnMinutesUp = nullptr;
    }
    uint8_t secondsToSet = 0;
    uint8_t minutesToSet = 0;
    Controllers::TimerController& timerController;
    lv_obj_t* time;
    lv_obj_t* msecTime;
    lv_obj_t* btnPlayPause;
    lv_obj_t* txtPlayPause;
    lv_obj_t* btnMinutesUp;
    lv_obj_t* btnMinutesDown;
    lv_obj_t* btnSecondsUp;
    lv_obj_t* btnSecondsDown;
    lv_obj_t* txtMUp;
    lv_obj_t* txtMDown;
    lv_obj_t* txtSUp;
    lv_obj_t* txtSDown;
    lv_task_t* taskRefresh;
  };
}
