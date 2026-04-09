#ifndef HYPRVIEW_GESTURE_MANAGER_HPP
#define HYPRVIEW_GESTURE_MANAGER_HPP

#include "Hyprview.hpp"
#include <hyprland/src/managers/input/Swipe.hpp>

class CGestureManager {
public:
  static void init();
  static void destroy();

  static void onSwipeBegin(void *data, std::any gestureData);
  static void onSwipeUpdate(void *data, std::any gestureData);
  static void onSwipeEnd(void *data, std::any gestureData);

private:
  struct SGestureState {
    bool active;
    int monitorId;
    float startProgress;
    float totalDelta;
    bool opening;
  };

  static SGestureState s_GestureState;
  static CEventHook *s_pSwipeBeginHook;
  static CEventHook *s_pSwipeUpdateHook;
  static CEventHook *s_pSwipeEndHook;
};

#endif
