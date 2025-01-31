#include "displayapp/screens/FlashLight.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/screens/Symbols.h"

using namespace Pinetime::Applications::Screens;

namespace {
  typedef Pinetime::Controllers::BrightnessController::Levels Levels;
}

void FlashLight::EventHandler(lv_obj_t* obj, lv_event_t event) {
  auto* screen = static_cast<FlashLight*>(obj->user_data);
  screen->OnClickEvent(obj, event);
}

FlashLight::FlashLight(Pinetime::Applications::DisplayApp* app,
                       System::SystemTask& systemTask,
                       Controllers::BrightnessController& brightnessController,
                       lv_color_t color)
  : Screen(app),
    systemTask {systemTask},
    brightnessController {brightnessController},
    color {color}

{
  brightnessLevel = brightnessController.Level();

  flashLight = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(flashLight, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_sys_48);
  lv_label_set_text_static(flashLight, Symbols::highlight);
  lv_obj_align(flashLight, nullptr, LV_ALIGN_CENTER, 0, 0);

  for (auto& i : indicators) {
    i = lv_obj_create(lv_scr_act(), nullptr);
    lv_obj_set_size(i, 15, 10);
    lv_obj_set_style_local_border_width(i, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
  }

  lv_obj_align(indicators[1], flashLight, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
  lv_obj_align(indicators[0], indicators[1], LV_ALIGN_OUT_LEFT_MID, -8, 0);
  lv_obj_align(indicators[2], indicators[1], LV_ALIGN_OUT_RIGHT_MID, 8, 0);

  Update(true, brightnessLevel);

  backgroundAction = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(backgroundAction, LV_LABEL_LONG_CROP);
  lv_obj_set_size(backgroundAction, 240, 240);
  lv_obj_set_pos(backgroundAction, 0, 0);
  lv_label_set_text_static(backgroundAction, "");
  lv_obj_set_click(backgroundAction, true);
  backgroundAction->user_data = this;
  lv_obj_set_event_cb(backgroundAction, EventHandler);
  systemTask.PushMessage(Pinetime::System::Messages::DisableSleeping);
}

FlashLight::~FlashLight() {
  Update(false, brightnessLevel);
  lv_obj_clean(lv_scr_act());
  systemTask.PushMessage(Pinetime::System::Messages::EnableSleeping);
}

void FlashLight::Update(bool on, Controllers::BrightnessController::Levels level) {
  brightnessLevel = level;
  
  if (brightnessLevel == Controllers::BrightnessController::Levels::High) {
    lv_obj_set_state(indicators[1], LV_STATE_DEFAULT);
    lv_obj_set_state(indicators[2], LV_STATE_DEFAULT);
  } else if (brightnessLevel == Controllers::BrightnessController::Levels::Medium) {
    lv_obj_set_state(indicators[1], LV_STATE_DEFAULT);
    lv_obj_set_state(indicators[2], LV_STATE_DISABLED);
  } else {
    lv_obj_set_state(indicators[1], LV_STATE_DISABLED);
    lv_obj_set_state(indicators[2], LV_STATE_DISABLED);
  }

  if (on != isOn) {
    decltype(color) color1, color2;
    isOn = on;
    if (isOn) {
      brightnessController.Backup();
      color1 = color;
      color2 = LV_COLOR_GRAY;
    } else {
      brightnessController.Restore();
      color1 = LV_COLOR_BLACK;
      color2 = color;
    }
    lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, color1);
    lv_obj_set_style_local_text_color(flashLight, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color2);
    for (auto& i : indicators) {
      lv_obj_set_style_local_bg_color(i, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, color2);
      lv_obj_set_style_local_bg_color(i, LV_OBJ_PART_MAIN, LV_STATE_DISABLED, color1);
      lv_obj_set_style_local_border_color(i, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, color2);
    }
  }

  if (isOn) {
    brightnessController.Set(brightnessLevel);
  }
}

bool FlashLight::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  // using namespace Pinetime::Controllers;
  auto level = static_cast< std::underlying_type_t<Levels> >(brightnessLevel);

  switch (event) {
    case TouchEvents::SwipeLeft:
      if (brightnessLevel >= Levels::Medium) level--;
      break;
    case TouchEvents::SwipeRight:
      if (brightnessLevel <= Levels::Medium) level++;
      break;
    default:
      return false;
  }

  Update(isOn, static_cast<Levels>(level));
  return true;
}

