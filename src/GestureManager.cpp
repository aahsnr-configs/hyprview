#include "GestureManager.hpp"
#include <hyprland/src/managers/input/InputManager.hpp>

CGestureManager::SGestureState CGestureManager::s_GestureState = {
    false, -1, 0.0f, 0.0f, false};
CEventHook *CGestureManager::s_pSwipeBeginHook = nullptr;
CEventHook *CGestureManager::s_pSwipeUpdateHook = nullptr;
CEventHook *CGestureManager::s_pSwipeEndHook = nullptr;

void CGestureManager::init() {
  s_pSwipeBeginHook =
      HyprlandAPI::createEventHook(PHANDLE, "swipeBegin", onSwipeBegin);
  s_pSwipeUpdateHook =
      HyprlandAPI::createEventHook(PHANDLE, "swipeUpdate", onSwipeUpdate);
  s_pSwipeEndHook =
      HyprlandAPI::createEventHook(PHANDLE, "swipeEnd", onSwipeEnd);
}

void CGestureManager::destroy() {
  if (s_pSwipeBeginHook) {
    s_pSwipeBeginHook->unhook();
    delete s_pSwipeBeginHook;
  }
  if (s_pSwipeUpdateHook) {
    s_pSwipeUpdateHook->unhook();
    delete s_pSwipeUpdateHook;
  }
  if (s_pSwipeEndHook) {
    s_pSwipeEndHook->unhook();
    delete s_pSwipeEndHook;
  }
}

void CGestureManager::onSwipeBegin(void *data, std::any gestureData) {
  auto pSwipe = std::any_cast<IPointer::SSwipeBegin>(gestureData);
  if (pSwipe.fingers != g_pHyprview->getConfig().gesture_fingers)
    return;

  CMonitor *pMonitor = g_pCompositor->getMonitorFromVector(pSwipe.pos);
  if (!pMonitor)
    return;

  s_GestureState.active = true;
  s_GestureState.monitorId = pMonitor->ID;
  s_GestureState.totalDelta = 0.0f;

  bool currentlyActive = g_pHyprview->isActive(pMonitor->ID);
  s_GestureState.opening = !currentlyActive;
  s_GestureState.startProgress = currentlyActive ? 1.0f : 0.0f;

  if (!currentlyActive) {
    g_pHyprview->beginGestureActivation(pMonitor->ID);
  }
}

void CGestureManager::onSwipeUpdate(void *data, std::any gestureData) {
  if (!s_GestureState.active)
    return;

  auto pSwipe = std::any_cast<IPointer::SSwipeUpdate>(gestureData);
  s_GestureState.totalDelta += std::abs(pSwipe.delta.x);

  float progressDelta =
      std::abs(pSwipe.delta.x) / g_pHyprview->getConfig().gesture_distance;
  float newProgress;

  if (s_GestureState.opening) {
    newProgress = std::min(1.0f, s_GestureState.startProgress + progressDelta);
  } else {
    newProgress = std::max(0.0f, s_GestureState.startProgress - progressDelta);
  }

  g_pHyprview->updateGestureActivation(s_GestureState.monitorId, newProgress);
}

void CGestureManager::onSwipeEnd(void *data, std::any gestureData) {
  if (!s_GestureState.active)
    return;

  auto pSwipe = std::any_cast<IPointer::SSwipeEnd>(gestureData);
  float threshold = g_pHyprview->getConfig().gesture_distance * 0.5f;
  bool committed = s_GestureState.totalDelta >= threshold;

  g_pHyprview->commitGestureActivation(s_GestureState.monitorId,
                                       s_GestureState.opening ? committed
                                                              : !committed);

  s_GestureState.active = false;
}
