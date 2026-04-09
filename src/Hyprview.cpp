#include "Hyprview.hpp"
#include "GestureManager.hpp"
#include <algorithm>
#include <cmath>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>

CHyprview *CHyprview::s_pInstance = nullptr;

float CHyprview::easeInOutCubic(float t) {
  if (t < 0.5f) {
    return 4.0f * t * t * t;
  } else {
    float f = t - 1.0f;
    return 1.0f + f * f * f * 4.0f;
  }
}

float CHyprview::applyAnimationCurve(float t, const std::string &curve) {
  if (curve == "linear")
    return t;
  if (curve == "easeInOutCubic")
    return easeInOutCubic(t);
  return easeInOutCubic(t);
}

CHyprview::CHyprview()
    : m_phHandle(nullptr), m_pRenderWorkspaceHook(nullptr),
      m_pMouseButtonHook(nullptr), m_pMouseMoveHook(nullptr),
      m_pKeyboardKeyHook(nullptr), m_pMonitorFrameHook(nullptr),
      m_pWindowDamageHook(nullptr), m_pWindowCommitHook(nullptr),
      m_pWindowDestroyHook(nullptr) {
  s_pInstance = this;
}

CHyprview::~CHyprview() { s_pInstance = nullptr; }

CHyprview *CHyprview::get() { return s_pInstance; }

SMonitorOverviewState &CHyprview::getMonitorState(int monitorId) {
  auto it = m_MonitorStates.find(monitorId);
  if (it == m_MonitorStates.end()) {
    SMonitorOverviewState newState;
    newState.monitorId = monitorId;
    newState.animProgress = g_pAnimationManager->createAnimation<float>(
        0.0f, nullptr, nullptr, nullptr, AVARDAMAGE_ENTIRE);
    auto result = m_MonitorStates.emplace(monitorId, std::move(newState));
    return result.first->second;
  }
  return it->second;
}

void CHyprview::init(HANDLE handle) {
  m_phHandle = handle;

  // Register configuration
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:columns",
                              Hyprlang::INT{3});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:gap_size",
                              Hyprlang::INT{5});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:border_size",
                              Hyprlang::INT{2});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:border_radius",
                              Hyprlang::INT{4});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:workspace_spacing",
                              Hyprlang::INT{40});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:zoom_level",
                              Hyprlang::FLOAT{0.7f});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:skip_empty",
                              Hyprlang::INT{1});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:layout_mode",
                              Hyprlang::STRING{"grid"});

  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:bg_color",
                              Hyprlang::INT{0xFF000000});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:dim_opacity",
                              Hyprlang::FLOAT{0.6f});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:border_color_active",
                              Hyprlang::INT{0xFF5294E2});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:border_color_inactive",
                              Hyprlang::INT{0xFF7A7A7A});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:hover_border_color",
                              Hyprlang::INT{0xFFFFFFFF});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:workspace_label_color",
                              Hyprlang::INT{0xFFFFFFFF});

  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:animation_duration",
                              Hyprlang::FLOAT{0.25f});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:animation_curve",
                              Hyprlang::STRING{"easeInOutCubic"});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:hover_scale",
                              Hyprlang::FLOAT{1.02f});

  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:gestures_enabled",
                              Hyprlang::INT{1});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:gesture_fingers",
                              Hyprlang::INT{4});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:gesture_distance",
                              Hyprlang::INT{300});

  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:max_fps_capture",
                              Hyprlang::INT{30});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:max_captures_per_frame",
                              Hyprlang::INT{2});

  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:close_on_click_empty",
                              Hyprlang::INT{1});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprview:show_workspace_labels",
                              Hyprlang::INT{1});
  HyprlandAPI::addConfigValue(
      PHANDLE, "plugin:hyprview:workspace_label_font_size", Hyprlang::INT{14});

  // Load config values
  m_Config.columns = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:columns")
          ->getValue());
  m_Config.gap_size = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:gap_size")
          ->getValue());
  m_Config.border_size = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:border_size")
          ->getValue());
  m_Config.border_radius = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:border_radius")
          ->getValue());
  m_Config.workspace_spacing = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:workspace_spacing")
          ->getValue());
  m_Config.zoom_level = std::any_cast<Hyprlang::FLOAT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:zoom_level")
          ->getValue());
  m_Config.skip_empty =
      std::any_cast<Hyprlang::INT>(
          HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:skip_empty")
              ->getValue()) != 0;
  m_Config.layout_mode = std::any_cast<Hyprlang::STRING>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:layout_mode")
          ->getValue());

  int bgColInt = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:bg_color")
          ->getValue());
  m_Config.bg_color = CColor(bgColInt);
  m_Config.dim_opacity = std::any_cast<Hyprlang::FLOAT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:dim_opacity")
          ->getValue());
  int activeCol = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE,
                                  "plugin:hyprview:border_color_active")
          ->getValue());
  m_Config.border_color_active = CColor(activeCol);
  int inactiveCol = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE,
                                  "plugin:hyprview:border_color_inactive")
          ->getValue());
  m_Config.border_color_inactive = CColor(inactiveCol);
  int hoverCol = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:hover_border_color")
          ->getValue());
  m_Config.hover_border_color = CColor(hoverCol);
  int labelCol = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE,
                                  "plugin:hyprview:workspace_label_color")
          ->getValue());
  m_Config.workspace_label_color = CColor(labelCol);

  m_Config.animation_duration = std::any_cast<Hyprlang::FLOAT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:animation_duration")
          ->getValue());
  m_Config.animation_curve = std::any_cast<Hyprlang::STRING>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:animation_curve")
          ->getValue());
  m_Config.hover_scale = std::any_cast<Hyprlang::FLOAT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:hover_scale")
          ->getValue());

  m_Config.gestures_enabled =
      std::any_cast<Hyprlang::INT>(
          HyprlandAPI::getConfigValue(PHANDLE,
                                      "plugin:hyprview:gestures_enabled")
              ->getValue()) != 0;
  m_Config.gesture_fingers = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:gesture_fingers")
          ->getValue());
  m_Config.gesture_distance = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:gesture_distance")
          ->getValue());

  m_Config.max_fps_capture = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprview:max_fps_capture")
          ->getValue());
  m_Config.max_captures_per_frame = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE,
                                  "plugin:hyprview:max_captures_per_frame")
          ->getValue());

  m_Config.close_on_click_empty =
      std::any_cast<Hyprlang::INT>(
          HyprlandAPI::getConfigValue(PHANDLE,
                                      "plugin:hyprview:close_on_click_empty")
              ->getValue()) != 0;
  m_Config.show_workspace_labels =
      std::any_cast<Hyprlang::INT>(
          HyprlandAPI::getConfigValue(PHANDLE,
                                      "plugin:hyprview:show_workspace_labels")
              ->getValue()) != 0;
  m_Config.workspace_label_font_size = std::any_cast<Hyprlang::INT>(
      HyprlandAPI::getConfigValue(PHANDLE,
                                  "plugin:hyprview:workspace_label_font_size")
          ->getValue());

  // Register dispatchers
  HyprlandAPI::addDispatcher(PHANDLE, "hyprview:toggle", [](std::string arg) {
    CHyprview::get()->toggle();
  });
  HyprlandAPI::addDispatcher(PHANDLE, "hyprview:open", [](std::string arg) {
    CHyprview::get()->activate(-1);
  });
  HyprlandAPI::addDispatcher(PHANDLE, "hyprview:close", [](std::string arg) {
    CHyprview::get()->deactivate(-1);
  });

  // Hooks
  static const auto RENDER_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "renderWorkspace");
  if (!RENDER_METHODS.empty()) {
    m_pRenderWorkspaceHook = HyprlandAPI::createFunctionHook(
        PHANDLE, RENDER_METHODS[0].address,
        (void *)[](void *thisptr, CMonitor *pMonitor, PHLWORKSPACE pWorkspace,
                   const CBox &box) {
          CHyprview::get()->onRenderWorkspace(thisptr, pMonitor, pWorkspace,
                                              box);
        });
    m_pRenderWorkspaceHook->hook();
  }

  static const auto MOUSE_BUTTON_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "onMouseButton");
  if (!MOUSE_BUTTON_METHODS.empty()) {
    m_pMouseButtonHook = HyprlandAPI::createFunctionHook(
        PHANDLE, MOUSE_BUTTON_METHODS[0].address,
        (void *)[](void *thisptr, wlr_pointer_button_event *e) {
          if (CHyprview::get()->onMouseButton(e)) {
            return;
          }
          ((origMouseButton)m_pMouseButtonHook->m_pOriginal)(thisptr, e);
        });
    m_pMouseButtonHook->hook();
  }

  static const auto MOUSE_MOVE_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "onMouseMove");
  if (!MOUSE_MOVE_METHODS.empty()) {
    m_pMouseMoveHook = HyprlandAPI::createFunctionHook(
        PHANDLE, MOUSE_MOVE_METHODS[0].address,
        (void *)[](void *thisptr, const Vector2D &pos) {
          CHyprview::get()->onMouseMove(pos);
          ((origMouseMove)m_pMouseMoveHook->m_pOriginal)(thisptr, pos);
        });
    m_pMouseMoveHook->hook();
  }

  static const auto KEYBOARD_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "onKeyboardKey");
  if (!KEYBOARD_METHODS.empty()) {
    m_pKeyboardKeyHook = HyprlandAPI::createFunctionHook(
        PHANDLE, KEYBOARD_METHODS[0].address,
        (void *)[](void *thisptr, wlr_keyboard_key_event *e) {
          if (CHyprview::get()->onKeyboardKey(e)) {
            return;
          }
          ((origKeyboardKey)m_pKeyboardKeyHook->m_pOriginal)(thisptr, e);
        });
    m_pKeyboardKeyHook->hook();
  }

  static const auto MONITOR_FRAME_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "listener_monitorFrame");
  if (!MONITOR_FRAME_METHODS.empty()) {
    m_pMonitorFrameHook = HyprlandAPI::createFunctionHook(
        PHANDLE, MONITOR_FRAME_METHODS[0].address,
        (void *)[](void *owner, void *data) {
          CHyprview::get()->onMonitorFrame(owner, data);
        });
    m_pMonitorFrameHook->hook();
  }

  static const auto DAMAGE_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "addDamage");
  if (!DAMAGE_METHODS.empty()) {
    m_pWindowDamageHook = HyprlandAPI::createFunctionHook(
        PHANDLE, DAMAGE_METHODS[0].address,
        (void *)[](void *thisptr, const CBox *box) {
          CHyprview::get()->onWindowDamage(thisptr, box);
          ((origWindowDamage)m_pWindowDamageHook->m_pOriginal)(thisptr, box);
        });
    m_pWindowDamageHook->hook();
  }

  static const auto COMMIT_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "onCommit");
  if (!COMMIT_METHODS.empty()) {
    m_pWindowCommitHook = HyprlandAPI::createFunctionHook(
        PHANDLE, COMMIT_METHODS[0].address, (void *)[](void *thisptr) {
          CHyprview::get()->onWindowCommit(thisptr);
          ((origWindowCommit)m_pWindowCommitHook->m_pOriginal)(thisptr);
        });
    m_pWindowCommitHook->hook();
  }

  static const auto DESTROY_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "removeWindowFromVectorSafe");
  if (!DESTROY_METHODS.empty()) {
    m_pWindowDestroyHook = HyprlandAPI::createFunctionHook(
        PHANDLE, DESTROY_METHODS[0].address,
        (void *)[](void *thisptr, PHLWINDOW w) {
          CHyprview::get()->onWindowDestroy(w.get());
          ((origWindowDestroy)m_pWindowDestroyHook->m_pOriginal)(thisptr, w);
        });
    m_pWindowDestroyHook->hook();
  }

  if (m_Config.gestures_enabled) {
    CGestureManager::init();
  }

  g_pHyprview = this;
  HyprlandAPI::addNotification(PHANDLE, "[hyprview] Loaded successfully!",
                               CColor(0.2, 1.0, 0.2, 1.0), 3000);
}

void CHyprview::shutdown() {
  CGestureManager::destroy();

  if (m_pRenderWorkspaceHook) {
    m_pRenderWorkspaceHook->unhook();
    delete m_pRenderWorkspaceHook;
  }
  if (m_pMouseButtonHook) {
    m_pMouseButtonHook->unhook();
    delete m_pMouseButtonHook;
  }
  if (m_pMouseMoveHook) {
    m_pMouseMoveHook->unhook();
    delete m_pMouseMoveHook;
  }
  if (m_pKeyboardKeyHook) {
    m_pKeyboardKeyHook->unhook();
    delete m_pKeyboardKeyHook;
  }
  if (m_pMonitorFrameHook) {
    m_pMonitorFrameHook->unhook();
    delete m_pMonitorFrameHook;
  }
  if (m_pWindowDamageHook) {
    m_pWindowDamageHook->unhook();
    delete m_pWindowDamageHook;
  }
  if (m_pWindowCommitHook) {
    m_pWindowCommitHook->unhook();
    delete m_pWindowCommitHook;
  }
  if (m_pWindowDestroyHook) {
    m_pWindowDestroyHook->unhook();
    delete m_pWindowDestroyHook;
  }

  for (auto &[id, state] : m_MonitorStates) {
    cleanupMonitorState(state);
  }
  m_MonitorStates.clear();
}

void CHyprview::cleanupMonitorState(SMonitorOverviewState &state) {
  for (auto &ws : state.workspaces) {
    for (auto &win : ws.windows) {
      if (win.framebuffer)
        win.framebuffer->release();
      win.texture = nullptr;
    }
  }
  state.workspaces.clear();
  state.state = OVERVIEW_INACTIVE;
}

void CHyprview::toggle(int monitorId) {
  if (monitorId < 0) {
    CMonitor *pMonitor = g_pCompositor->m_pLastMonitor;
    if (!pMonitor)
      return;
    monitorId = pMonitor->ID;
  }
  auto &state = getMonitorState(monitorId);
  if (state.state == OVERVIEW_INACTIVE) {
    activate(monitorId);
  } else {
    deactivate(monitorId);
  }
}

void CHyprview::activate(int monitorId) {
  if (monitorId < 0) {
    CMonitor *pMonitor = g_pCompositor->m_pLastMonitor;
    if (!pMonitor)
      return;
    monitorId = pMonitor->ID;
  }
  auto &state = getMonitorState(monitorId);
  if (state.state != OVERVIEW_INACTIVE)
    return;

  state.state = OVERVIEW_ENTERING;
  *state.animProgress = 0.0f;
  state.animProgress->setValueAndWarp(0.0f);
  state.animProgress->setConfig(m_Config.animation_duration,
                                m_Config.animation_curve);

  captureWorkspaces(monitorId);
  computeLayout(monitorId);

  CMonitor *pMonitor = g_pCompositor->getMonitorFromID(monitorId);
  if (pMonitor)
    g_pHyprRenderer->damageMonitor(pMonitor);
}

void CHyprview::deactivate(int monitorId) {
  if (monitorId < 0) {
    CMonitor *pMonitor = g_pCompositor->m_pLastMonitor;
    if (!pMonitor)
      return;
    monitorId = pMonitor->ID;
  }
  auto it = m_MonitorStates.find(monitorId);
  if (it == m_MonitorStates.end())
    return;
  auto &state = it->second;

  if (state.state == OVERVIEW_INACTIVE)
    return;
  state.state = OVERVIEW_EXITING;
  state.animProgress->setConfig(m_Config.animation_duration,
                                m_Config.animation_curve);
}

bool CHyprview::isActive(int monitorId) const {
  if (monitorId < 0)
    return false;
  auto it = m_MonitorStates.find(monitorId);
  if (it == m_MonitorStates.end())
    return false;
  return it->second.state != OVERVIEW_INACTIVE;
}

eOverviewState CHyprview::getState(int monitorId) const {
  auto it = m_MonitorStates.find(monitorId);
  if (it == m_MonitorStates.end())
    return OVERVIEW_INACTIVE;
  return it->second.state;
}

void CHyprview::captureWorkspaces(int monitorId) {
  auto &state = getMonitorState(monitorId);
  state.workspaces.clear();

  for (auto &ws : g_pCompositor->m_vWorkspaces) {
    if (!ws || ws->m_iMonitorID != monitorId)
      continue;
    if (m_Config.skip_empty && ws->m_vWindows.empty())
      continue;

    SWorkspaceSnapshot snapshot;
    snapshot.workspace = ws;
    snapshot.workspaceId = ws->m_iID;
    snapshot.monitorId = monitorId;
    snapshot.needsLayout = true;

    for (auto &w : ws->m_vWindows) {
      if (!w || w->m_bHidden)
        continue;

      SWindowSnapshot winSnap;
      winSnap.window = w;
      winSnap.originalGeometry = {
          w->m_vRealPosition.goalv().x, w->m_vRealPosition.goalv().y,
          w->m_vRealSize.goalv().x, w->m_vRealSize.goalv().y};
      winSnap.workspaceId = ws->m_iID;
      winSnap.monitorId = monitorId;
      winSnap.isActive = (w == g_pCompositor->m_pLastWindow.lock());
      winSnap.needsUpdate = true;
      winSnap.lastCaptureTime = 0.0f;
      winSnap.damageSubscription = nullptr;

      winSnap.animPosition = g_pAnimationManager->createAnimation<Vector2D>(
          Vector2D{winSnap.originalGeometry.x, winSnap.originalGeometry.y},
          nullptr, nullptr, nullptr, AVARDAMAGE_ENTIRE);
      winSnap.animSize = g_pAnimationManager->createAnimation<Vector2D>(
          Vector2D{winSnap.originalGeometry.width,
                   winSnap.originalGeometry.height},
          nullptr, nullptr, nullptr, AVARDAMAGE_ENTIRE);

      snapshot.windows.push_back(winSnap);
      state.captureQueue.push_back(&snapshot.windows.back());
    }

    state.workspaces.push_back(snapshot);
  }
}

void CHyprview::computeLayout(int monitorId) {
  auto &state = getMonitorState(monitorId);
  CMonitor *pMonitor = g_pCompositor->getMonitorFromID(monitorId);
  if (!pMonitor)
    return;

  int numWorkspaces = state.workspaces.size();
  if (numWorkspaces == 0)
    return;

  int columns = std::min(m_Config.columns, numWorkspaces);
  int rows = (numWorkspaces + columns - 1) / columns;

  float availableWidth = pMonitor->vecPixelSize.x * 0.85f;
  float availableHeight = pMonitor->vecPixelSize.y * 0.85f;

  float workspaceWidth =
      (availableWidth - (columns - 1) * m_Config.workspace_spacing) / columns;
  float workspaceHeight =
      (availableHeight - (rows - 1) * m_Config.workspace_spacing) / rows;

  float gridStartX = (pMonitor->vecPixelSize.x -
                      (columns * workspaceWidth +
                       (columns - 1) * m_Config.workspace_spacing)) /
                     2.0f;
  float gridStartY =
      (pMonitor->vecPixelSize.y -
       (rows * workspaceHeight + (rows - 1) * m_Config.workspace_spacing)) /
      2.0f;

  state.maxViewportOffset =
      std::max(0.0f, (columns * workspaceWidth +
                      (columns - 1) * m_Config.workspace_spacing) -
                         pMonitor->vecPixelSize.x);

  for (int i = 0; i < numWorkspaces; ++i) {
    int row = i / columns;
    int col = i % columns;

    auto &ws = state.workspaces[i];
    ws.layoutGeometry.x =
        gridStartX + col * (workspaceWidth + m_Config.workspace_spacing);
    ws.layoutGeometry.y =
        gridStartY + row * (workspaceHeight + m_Config.workspace_spacing);
    ws.layoutGeometry.width = workspaceWidth;
    ws.layoutGeometry.height = workspaceHeight;

    int numWindows = ws.windows.size();
    if (numWindows == 0)
      continue;

    int winCols = std::ceil(std::sqrt(numWindows));
    int winRows = std::ceil((float)numWindows / winCols);

    float cellWidth =
        (workspaceWidth - (winCols - 1) * m_Config.gap_size) / winCols;
    float cellHeight =
        (workspaceHeight - (winRows - 1) * m_Config.gap_size) / winRows;

    for (int j = 0; j < numWindows; ++j) {
      int wRow = j / winCols;
      int wCol = j % winCols;

      float baseX =
          workspaceWidth * 0.1f + wCol * (cellWidth + m_Config.gap_size);
      float baseY =
          workspaceHeight * 0.15f + wRow * (cellHeight + m_Config.gap_size);

      auto &win = ws.windows[j];
      float winAspect =
          win.originalGeometry.width / (float)win.originalGeometry.height;
      float cellAspect = cellWidth / cellHeight;

      float scaledWidth, scaledHeight;
      if (winAspect > cellAspect) {
        scaledWidth = cellWidth * m_Config.zoom_level;
        scaledHeight = scaledWidth / winAspect;
      } else {
        scaledHeight = cellHeight * m_Config.zoom_level;
        scaledWidth = scaledHeight * winAspect;
      }

      win.scaledGeometry.x = baseX + (cellWidth - scaledWidth) / 2.0f;
      win.scaledGeometry.y = baseY + (cellHeight - scaledHeight) / 2.0f;
      win.scaledGeometry.width = scaledWidth;
      win.scaledGeometry.height = scaledHeight;
    }
  }
  state.needsRedraw = true;
}

void CHyprview::captureWindowToFBO(SWindowSnapshot *pWindow,
                                   CMonitor *pMonitor) {
  if (!pWindow || !pWindow->window)
    return;

  auto window = pWindow->window;
  Vector2D size = window->m_vRealSize.goalv();
  if (size.x <= 0 || size.y <= 0)
    return;

  if (!pWindow->framebuffer) {
    pWindow->framebuffer = makeShared<CFramebuffer>();
  }

  if (!pWindow->framebuffer->alloc(size.x, size.y, true)) {
    Debug::log(ERR, "[hyprview] Failed to allocate FBO for window");
    return;
  }

  auto originalFBO = g_pHyprOpenGL->m_RenderData.pCurrentMonData->primaryFB;
  bool wasRenderingToFBO = g_pHyprRenderer->m_bRenderingToFBO;

  pWindow->framebuffer->bind();
  g_pHyprRenderer->m_bRenderingToFBO = true;
  g_pHyprOpenGL->m_RenderData.pCurrentMonData->primaryFB = pWindow->framebuffer;

  g_pHyprOpenGL->clear(CColor(0.0, 0.0, 0.0, 0.0));

  g_pHyprRenderer->renderWindow(window, pMonitor,
                                std::chrono::system_clock::now(), true,
                                RENDER_PASS_ALL);

  g_pHyprRenderer->m_bRenderingToFBO = wasRenderingToFBO;
  g_pHyprOpenGL->m_RenderData.pCurrentMonData->primaryFB = originalFBO;

  pWindow->texture = pWindow->framebuffer->getTexture();
  pWindow->needsUpdate = false;
  pWindow->lastCaptureTime =
      std::chrono::duration<float>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count();
}

void CHyprview::processCaptureQueue(SMonitorOverviewState &state) {
  int captures = 0;
  float currentTime = std::chrono::duration<float>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();
  float minInterval = 1.0f / m_Config.max_fps_capture;

  CMonitor *pMonitor = g_pCompositor->getMonitorFromID(state.monitorId);
  if (!pMonitor)
    return;

  for (auto it = state.captureQueue.begin();
       it != state.captureQueue.end() &&
       captures < m_Config.max_captures_per_frame;) {
    SWindowSnapshot *win = *it;
    if (win->needsUpdate &&
        (currentTime - win->lastCaptureTime) >= minInterval) {
      bool visible = false;
      for (auto &ws : state.workspaces) {
        CBox wsBox = ws.layoutGeometry;
        wsBox.x -= state.viewportOffset;
        CBox winBox = {wsBox.x + win->scaledGeometry.x,
                       wsBox.y + win->scaledGeometry.y,
                       win->scaledGeometry.width, win->scaledGeometry.height};
        CBox viewport = {state.viewportOffset, 0, pMonitor->vecPixelSize.x,
                         pMonitor->vecPixelSize.y};
        if (winBox.intersects(viewport)) {
          visible = true;
          break;
        }
      }
      if (visible) {
        captureWindowToFBO(win, pMonitor);
        captures++;
      }
      it = state.captureQueue.erase(it);
    } else {
      ++it;
    }
  }
}

void CHyprview::onRenderWorkspace(void *thisptr, CMonitor *pMonitor,
                                  PHLWORKSPACE pWorkspace, const CBox &box) {
  auto it = m_MonitorStates.find(pMonitor->ID);
  if (it == m_MonitorStates.end() || it->second.state == OVERVIEW_INACTIVE) {
    ((origRenderWorkspace)m_pRenderWorkspaceHook->m_pOriginal)(
        thisptr, pMonitor, pWorkspace, box);
    return;
  }
  renderOverview(pMonitor);
}

void CHyprview::renderOverview(CMonitor *pMonitor) {
  auto &state = getMonitorState(pMonitor->ID);
  if (state.state == OVERVIEW_INACTIVE)
    return;

  float rawProgress = *state.animProgress;
  float progress = applyAnimationCurve(rawProgress, m_Config.animation_curve);
  float bgAlpha = m_Config.dim_opacity * progress;

  CBox screenBox = {{0, 0}, pMonitor->vecPixelSize};
  g_pHyprOpenGL->renderRect(&screenBox, CHyprColor{0.0f, 0.0f, 0.0f, bgAlpha},
                            0);

  for (auto &ws : state.workspaces) {
    CBox wsBox = ws.layoutGeometry;
    wsBox.x -= state.viewportOffset;

    g_pHyprOpenGL->renderRect(&wsBox, m_Config.bg_color,
                              m_Config.border_radius);

    if (m_Config.show_workspace_labels) {
      std::string label = "Workspace " + std::to_string(ws.workspaceId);
      float fontSize = m_Config.workspace_label_font_size * pMonitor->scale;
      auto textExtent =
          g_pHyprOpenGL->renderText(nullptr, label, CColor(), fontSize);
      Vector2D labelPos = {wsBox.x + (wsBox.width - textExtent.x) / 2.0f,
                           wsBox.y - textExtent.y - 5.0f};
      g_pHyprOpenGL->renderText(labelPos, label, m_Config.workspace_label_color,
                                fontSize);
    }

    for (auto &win : ws.windows) {
      Vector2D origPos{win.originalGeometry.x, win.originalGeometry.y};
      Vector2D origSize{win.originalGeometry.width,
                        win.originalGeometry.height};
      Vector2D targetPos{wsBox.x + win.scaledGeometry.x,
                         wsBox.y + win.scaledGeometry.y};
      Vector2D targetSize{win.scaledGeometry.width, win.scaledGeometry.height};

      Vector2D curPos = origPos * (1.0f - progress) + targetPos * progress;
      Vector2D curSize = origSize * (1.0f - progress) + targetSize * progress;

      CBox winBox{curPos.x, curPos.y, curSize.x, curSize.y};

      bool isHovered = (state.hoveredWindow == &win);
      if (isHovered && m_Config.hover_scale > 1.0f) {
        float scale = 1.0f + (m_Config.hover_scale - 1.0f) * progress;
        float centerX = winBox.x + winBox.width / 2.0f;
        float centerY = winBox.y + winBox.height / 2.0f;
        winBox.width *= scale;
        winBox.height *= scale;
        winBox.x = centerX - winBox.width / 2.0f;
        winBox.y = centerY - winBox.height / 2.0f;
      }

      if (win.texture && !win.needsUpdate) {
        g_pHyprOpenGL->renderTexture(winBox, win.texture, 1.0f,
                                     m_Config.border_radius);
      } else {
        CColor placeholder = win.isActive ? CColor(0.2f, 0.4f, 0.8f, 0.8f)
                                          : CColor(0.3f, 0.3f, 0.3f, 0.8f);
        g_pHyprOpenGL->renderRect(&winBox, placeholder, m_Config.border_radius);
        if (win.needsUpdate) {
          bool queued = false;
          for (auto q : state.captureQueue)
            if (q == &win) {
              queued = true;
              break;
            }
          if (!queued)
            const_cast<std::vector<SWindowSnapshot *> &>(state.captureQueue)
                .push_back(const_cast<SWindowSnapshot *>(&win));
        }
      }

      CColor borderCol = win.isActive ? m_Config.border_color_active
                                      : m_Config.border_color_inactive;
      if (isHovered)
        borderCol = m_Config.hover_border_color;
      CBox borderBox = winBox;
      borderBox.x -= m_Config.border_size;
      borderBox.y -= m_Config.border_size;
      borderBox.width += m_Config.border_size * 2;
      borderBox.height += m_Config.border_size * 2;
      g_pHyprOpenGL->renderBorder(&borderBox, borderCol, m_Config.border_size,
                                  m_Config.border_radius);
    }
  }

  if (state.dragState.active && state.dragState.window) {
    CBox dragBox{state.dragState.currentPos.x - 50,
                 state.dragState.currentPos.y - 30, 100, 60};
    g_pHyprOpenGL->renderRect(&dragBox, CHyprColor{0.5f, 0.5f, 0.5f, 0.7f}, 4);
  }

  g_pHyprRenderer->damageMonitor(pMonitor);
}

bool CHyprview::onMouseButton(wlr_pointer_button_event *e) {
  Vector2D cursorPos = g_pInputManager->getMouseCoords();
  CMonitor *pMonitor = g_pCompositor->getMonitorFromVector(cursorPos);
  if (!pMonitor)
    return false;

  auto it = m_MonitorStates.find(pMonitor->ID);
  if (it == m_MonitorStates.end() || it->second.state != OVERVIEW_ACTIVE)
    return false;
  auto &state = it->second;

  if (e->state == WLR_BUTTON_PRESSED) {
    if (e->button == BTN_LEFT) {
      SWindowSnapshot *pWin = getWindowAtCursor(state, cursorPos);
      if (pWin) {
        state.dragState.active = true;
        state.dragState.window = pWin;
        state.dragState.startPos = cursorPos;
        state.dragState.currentPos = cursorPos;
        state.dragState.sourceMonitor = pMonitor->ID;
        return true;
      } else if (m_Config.close_on_click_empty) {
        deactivate(pMonitor->ID);
        return true;
      }
    } else if (e->button == BTN_RIGHT) {
      state.scrollState.active = true;
      state.scrollState.startX = cursorPos.x;
      return true;
    }
  } else if (e->state == WLR_BUTTON_RELEASED) {
    if (e->button == BTN_LEFT && state.dragState.active) {
      SWorkspaceSnapshot *pTargetWs = getWorkspaceAtCursor(state, cursorPos);
      if (pTargetWs && state.dragState.window) {
        moveWindowToWorkspace(state.dragState.window, pTargetWs);
      }
      state.dragState.active = false;
      return true;
    } else if (e->button == BTN_RIGHT) {
      state.scrollState.active = false;
      return true;
    }
  }
  return false;
}

void CHyprview::onMouseMove(const Vector2D &pos) {
  CMonitor *pMonitor = g_pCompositor->getMonitorFromVector(pos);
  if (!pMonitor)
    return;

  auto it = m_MonitorStates.find(pMonitor->ID);
  if (it == m_MonitorStates.end() || it->second.state != OVERVIEW_ACTIVE)
    return;
  auto &state = it->second;

  state.hoveredWindow = getWindowAtCursor(state, pos);

  if (state.dragState.active) {
    state.dragState.currentPos = pos;
    if (pMonitor->ID != state.dragState.sourceMonitor) {
      if (!hasMonitorState(pMonitor->ID) ||
          getMonitorState(pMonitor->ID).state == OVERVIEW_INACTIVE) {
        activate(pMonitor->ID);
      }
      state.dragState.sourceMonitor = pMonitor->ID;
    }
  }

  if (state.scrollState.active) {
    float deltaX = pos.x - state.scrollState.startX;
    scrollWorkspaces(state, -deltaX);
    state.scrollState.startX = pos.x;
  }
}

bool CHyprview::onKeyboardKey(wlr_keyboard_key_event *e) {
  CMonitor *pMonitor = g_pCompositor->m_pLastMonitor;
  if (!pMonitor)
    return false;

  auto it = m_MonitorStates.find(pMonitor->ID);
  if (it == m_MonitorStates.end() || it->second.state != OVERVIEW_ACTIVE)
    return false;
  auto &state = it->second;

  if (e->state == WLR_KEY_PRESSED) {
    switch (e->keycode) {
    case KEY_ESC:
      deactivate(pMonitor->ID);
      return true;
    case KEY_RETURN:
    case KEY_SPACE:
      if (state.hoveredWindow) {
        g_pCompositor->focusWindow(state.hoveredWindow->window);
        deactivate(pMonitor->ID);
      }
      return true;
    case KEY_LEFT:
    case KEY_H:
      navigateWindows(state, 0);
      return true;
    case KEY_RIGHT:
    case KEY_L:
      navigateWindows(state, 1);
      return true;
    case KEY_UP:
    case KEY_K:
      navigateWindows(state, 2);
      return true;
    case KEY_DOWN:
    case KEY_J:
      navigateWindows(state, 3);
      return true;
    case KEY_TAB:
      if (state.centeredWorkspace) {
        for (size_t i = 0; i < state.workspaces.size(); ++i) {
          if (&state.workspaces[i] == state.centeredWorkspace) {
            size_t next = (i + 1) % state.workspaces.size();
            state.centeredWorkspace = &state.workspaces[next];
            CBox wsBox = state.centeredWorkspace->layoutGeometry;
            state.viewportOffset = std::clamp(
                wsBox.x - (pMonitor->vecPixelSize.x - wsBox.width) / 2.0f, 0.0f,
                state.maxViewportOffset);
            break;
          }
        }
      }
      return true;
    }
  }
  return false;
}

void CHyprview::onMonitorFrame(void *owner, void *data) {
  ((origMonitorFrame)m_pMonitorFrameHook->m_pOriginal)(owner, data);

  float currentTime = std::chrono::duration<float>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();

  for (auto &[id, state] : m_MonitorStates) {
    if (state.state == OVERVIEW_ENTERING || state.state == OVERVIEW_EXITING ||
        state.state == OVERVIEW_ACTIVE) {
      float deltaTime = currentTime - state.lastFrameTime;
      if (state.lastFrameTime > 0.0f) {
        updateAnimation(state, deltaTime);
      }
      state.lastFrameTime = currentTime;

      processCaptureQueue(state);

      CMonitor *pMonitor = g_pCompositor->getMonitorFromID(id);
      if (pMonitor && state.needsRedraw) {
        g_pHyprRenderer->damageMonitor(pMonitor);
        state.needsRedraw = false;
      }
    }
  }
}

void CHyprview::updateAnimation(SMonitorOverviewState &state, float deltaTime) {
  float step = deltaTime / m_Config.animation_duration;

  if (state.state == OVERVIEW_ENTERING) {
    *state.animProgress = std::min(1.0f, *state.animProgress + step);
    if (*state.animProgress >= 1.0f) {
      state.state = OVERVIEW_ACTIVE;
    }
  } else if (state.state == OVERVIEW_EXITING) {
    *state.animProgress = std::max(0.0f, *state.animProgress - step);
    if (*state.animProgress <= 0.0f) {
      cleanupMonitorState(state);
      state.state = OVERVIEW_INACTIVE;
    }
  }
  state.needsRedraw = true;
}

void CHyprview::onWindowDamage(void *windowPtr, const CBox *box) {
  for (auto &[id, state] : m_MonitorStates) {
    if (state.state == OVERVIEW_INACTIVE)
      continue;
    for (auto &ws : state.workspaces) {
      for (auto &win : ws.windows) {
        if (win.window.get() == windowPtr) {
          win.needsUpdate = true;
          bool found = false;
          for (auto q : state.captureQueue)
            if (q == &win) {
              found = true;
              break;
            }
          if (!found)
            const_cast<std::vector<SWindowSnapshot *> &>(state.captureQueue)
                .push_back(&win);
          return;
        }
      }
    }
  }
}

void CHyprview::onWindowCommit(void *windowPtr) {
  onWindowDamage(windowPtr, nullptr);
}

void CHyprview::onWindowDestroy(void *windowPtr) {
  for (auto &[id, state] : m_MonitorStates) {
    for (auto &ws : state.workspaces) {
      for (auto it = ws.windows.begin(); it != ws.windows.end();) {
        if (it->window.get() == windowPtr) {
          if (it->framebuffer)
            it->framebuffer->release();
          it = ws.windows.erase(it);
        } else {
          ++it;
        }
      }
    }
  }
}

SWindowSnapshot *CHyprview::getWindowAtCursor(SMonitorOverviewState &state,
                                              const Vector2D &cursorPos) {
  for (auto &ws : state.workspaces) {
    CBox wsBox = ws.layoutGeometry;
    wsBox.x -= state.viewportOffset;
    for (auto &win : ws.windows) {
      CBox winBox = {wsBox.x + win.scaledGeometry.x,
                     wsBox.y + win.scaledGeometry.y, win.scaledGeometry.width,
                     win.scaledGeometry.height};
      if (winBox.containsPoint(cursorPos)) {
        return &win;
      }
    }
  }
  return nullptr;
}

SWorkspaceSnapshot *
CHyprview::getWorkspaceAtCursor(SMonitorOverviewState &state,
                                const Vector2D &cursorPos) {
  for (auto &ws : state.workspaces) {
    CBox wsBox = ws.layoutGeometry;
    wsBox.x -= state.viewportOffset;
    if (wsBox.containsPoint(cursorPos)) {
      return &ws;
    }
  }
  return nullptr;
}

void CHyprview::moveWindowToWorkspace(SWindowSnapshot *pWindow,
                                      SWorkspaceSnapshot *pTargetWorkspace) {
  if (!pWindow || !pTargetWorkspace)
    return;
  std::string cmd =
      "movetoworkspace " + std::to_string(pTargetWorkspace->workspaceId);
  HyprlandAPI::invokeHyprctlCommand(cmd, "");
  captureWorkspaces(pTargetWorkspace->monitorId);
  computeLayout(pTargetWorkspace->monitorId);
}

void CHyprview::scrollWorkspaces(SMonitorOverviewState &state, float deltaX) {
  state.viewportOffset =
      std::clamp(state.viewportOffset + deltaX, 0.0f, state.maxViewportOffset);
  state.needsRedraw = true;
}

void CHyprview::navigateWindows(SMonitorOverviewState &state, int direction) {
  if (!state.hoveredWindow) {
    if (!state.workspaces.empty() && !state.workspaces[0].windows.empty())
      state.hoveredWindow = &state.workspaces[0].windows[0];
    return;
  }

  std::vector<std::pair<SWindowSnapshot *, Vector2D>> visible;
  CMonitor *pMonitor = g_pCompositor->getMonitorFromID(state.monitorId);
  if (!pMonitor)
    return;
  CBox viewport = {state.viewportOffset, 0, pMonitor->vecPixelSize.x,
                   pMonitor->vecPixelSize.y};

  for (auto &ws : state.workspaces) {
    CBox wsBox = ws.layoutGeometry;
    wsBox.x -= state.viewportOffset;
    for (auto &win : ws.windows) {
      CBox winBox = {wsBox.x + win.scaledGeometry.x,
                     wsBox.y + win.scaledGeometry.y, win.scaledGeometry.width,
                     win.scaledGeometry.height};
      if (winBox.intersects(viewport)) {
        Vector2D center = {winBox.x + winBox.width / 2.0f,
                           winBox.y + winBox.height / 2.0f};
        visible.push_back({&win, center});
      }
    }
  }

  if (visible.empty())
    return;

  Vector2D currentCenter = {0, 0};
  for (auto &[win, center] : visible) {
    if (win == state.hoveredWindow) {
      currentCenter = center;
      break;
    }
  }

  SWindowSnapshot *best = nullptr;
  float bestScore = INFINITY;

  for (auto &[win, center] : visible) {
    if (win == state.hoveredWindow)
      continue;
    Vector2D delta = center - currentCenter;

    bool valid = false;
    if (direction == 0 && delta.x < 0)
      valid = true;
    else if (direction == 1 && delta.x > 0)
      valid = true;
    else if (direction == 2 && delta.y < 0)
      valid = true;
    else if (direction == 3 && delta.y > 0)
      valid = true;

    if (!valid)
      continue;

    float primary = (direction < 2) ? std::abs(delta.x) : std::abs(delta.y);
    float secondary = (direction < 2) ? std::abs(delta.y) : std::abs(delta.x);
    float score = primary + secondary * 0.5f;

    if (score < bestScore) {
      bestScore = score;
      best = win;
    }
  }

  if (best) {
    state.hoveredWindow = best;
    ensureWindowVisible(state, best);
  }
}

void CHyprview::ensureWindowVisible(SMonitorOverviewState &state,
                                    SWindowSnapshot *window) {
  CMonitor *pMonitor = g_pCompositor->getMonitorFromID(state.monitorId);
  if (!pMonitor)
    return;

  for (auto &ws : state.workspaces) {
    for (auto &win : ws.windows) {
      if (&win == window) {
        CBox wsBox = ws.layoutGeometry;
        wsBox.x -= state.viewportOffset;
        CBox winBox = {wsBox.x + win.scaledGeometry.x,
                       wsBox.y + win.scaledGeometry.y, win.scaledGeometry.width,
                       win.scaledGeometry.height};

        if (winBox.x < 0) {
          state.viewportOffset += winBox.x - m_Config.workspace_spacing;
        } else if (winBox.x + winBox.width > pMonitor->vecPixelSize.x) {
          state.viewportOffset += (winBox.x + winBox.width) -
                                  pMonitor->vecPixelSize.x +
                                  m_Config.workspace_spacing;
        }
        state.viewportOffset =
            std::clamp(state.viewportOffset, 0.0f, state.maxViewportOffset);
        return;
      }
    }
  }
}

void CHyprview::beginGestureActivation(int monitorId) {
  auto &state = getMonitorState(monitorId);
  if (state.state == OVERVIEW_INACTIVE) {
    state.state = OVERVIEW_ENTERING;
    *state.animProgress = 0.0f;
    captureWorkspaces(monitorId);
    computeLayout(monitorId);
  }
}

void CHyprview::updateGestureActivation(int monitorId, float progress) {
  auto &state = getMonitorState(monitorId);
  *state.animProgress = std::clamp(progress, 0.0f, 1.0f);
  CMonitor *pMonitor = g_pCompositor->getMonitorFromID(monitorId);
  if (pMonitor)
    g_pHyprRenderer->damageMonitor(pMonitor);
}

void CHyprview::commitGestureActivation(int monitorId, bool activate) {
  if (activate) {
    this->activate(monitorId);
  } else {
    deactivate(monitorId);
  }
}
