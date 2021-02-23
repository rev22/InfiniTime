#pragma once

#include <memory>

#include "displayapp/screens/Screen.h"
#include "displayapp/screens/ScreenList.h"
#include "components/datetime/DateTimeController.h"
#include "components/settings/Settings.h"
#include "components/battery/BatteryController.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/Tile.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class ApplicationList : public Screen {
      public:
        explicit ApplicationList(DisplayApp* app,
                                 Pinetime::Controllers::Settings& settingsController,
                                 Pinetime::Controllers::Battery& batteryController,
                                 Controllers::DateTime& dateTimeController);
        ~ApplicationList() override;
        bool OnTouchEvent(TouchEvents event) override;

      private:
        auto CreateScreenList() const;
        std::unique_ptr<Screen> CreateScreen(unsigned int screenNum) const;

        Controllers::Settings& settingsController;
        Pinetime::Controllers::Battery& batteryController;
        Controllers::DateTime& dateTimeController;

        static constexpr int appsPerScreen = 6;
        static constexpr auto applications{[]() constexpr{
          constexpr Tile::Applications list[] = {
            {Symbols::stopWatch, Apps::StopWatch},
            {Symbols::clock, Apps::Alarm},
            {Symbols::hourGlass, Apps::Timer},
            {Symbols::shoe, Apps::Steps},
            {Symbols::heartBeat, Apps::HeartRate},
            {Symbols::music, Apps::Music},

            {Symbols::paintbrush, Apps::Paint},
            {Symbols::paddle, Apps::Paddle},
            {"2", Apps::Twos},
            {Symbols::chartLine, Apps::Motion},
            {Symbols::drum, Apps::Metronome},
            {Symbols::map, Apps::Navigation},

            {Symbols::calculator, Apps::Calculator},
          };
          std::array<std::array<Tile::Applications, appsPerScreen>, ((std::size(list) + appsPerScreen - 1) / appsPerScreen)> r{};;
          int idx = 0;
          for (auto& f : r) {
            for (auto& e : f) {
              if (idx < std::size(list)) {
                e = list[idx];
              } else {
                e = { Symbols::none, Apps::None };
              }
              idx++;
            }
          }
          return r;
        }()};
        static constexpr auto nScreens = std::size(applications);
        ScreenList<nScreens> screens;
      };
    }
  }
}
