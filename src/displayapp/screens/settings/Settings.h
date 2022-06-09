#pragma once

#include <cstdint>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/ScreenList.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/List.h"

namespace Pinetime {

  namespace Applications {
    namespace Screens {

      class Settings : public Screen {
      public:
        Settings(DisplayApp* app, Pinetime::Controllers::Settings& settingsController);
        ~Settings() override;

        bool OnTouchEvent(Pinetime::Applications::TouchEvents event) override;

      private:
        auto CreateScreenList() const;
        std::unique_ptr<Screen> CreateScreen(unsigned int screenNum) const;

        Controllers::Settings& settingsController;

        static constexpr int entriesPerScreen = 4;
        static constexpr auto entries{[]() constexpr{
          using namespace Pinetime::Applications::Screens;
          using namespace Pinetime::Applications;
          constexpr List::Applications list[] = {
            {Symbols::sun, "Display", Apps::SettingDisplay},
            {Symbols::eye, "Wake Up", Apps::SettingWakeUp},
            {Symbols::clock, "Time format", Apps::SettingTimeFormat},
            {Symbols::home, "Watch face", Apps::SettingWatchFace},

            {Symbols::shoe, "Steps", Apps::SettingSteps},
            {Symbols::clock, "Set date", Apps::SettingSetDate},
            {Symbols::clock, "Set time", Apps::SettingSetTime},
            {Symbols::batteryHalf, "Battery", Apps::BatteryInfo},

            {Symbols::clock, "Chimes", Apps::SettingChimes},
            {Symbols::tachometer, "Shake Calib.", Apps::SettingShakeThreshold},
            {Symbols::check, "Firmware", Apps::FirmwareValidation},
            {Symbols::bluetooth, "Bluetooth", Apps::SettingBluetooth},

            {Symbols::list, "About", Apps::SysInfo},
          };
          std::array<std::array<List::Applications, entriesPerScreen>, ((std::size(list) + entriesPerScreen - 1) / entriesPerScreen)> r{};;
          int idx = 0;
          for (auto& f : r) {
            for (auto& e : f) {
              if (idx < std::size(list)) {
                e = list[idx];
              } else {
                e = { Symbols::none, "",  Apps::None };
              }
              idx++;
            }
          }
          return r;
        }()};
        static constexpr auto nScreens = std::size(entries);
        ScreenList<nScreens> screens;
      };
    }
  }
}
