#ifndef HYPRVIEW_HPP
#define HYPRVIEW_HPP

#include "config.h"
#include <chrono>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprland/src/managers/AnimationManager.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

#define private public
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/managers/InputManager.hpp>
#include <hyprland/src/managers/SeatManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#undef private

using namespace HyprlandAPI;

enum eOverviewState {
  OVERVIEW_INACTIVE = 0,
  OVERVIEW_ENTERING,
  OVERVIEW_ACTIVE,
  OVERVIEW_EXITING
};

struct SWindowSnapshot {
  PHLWINDOW window;
  SP<CTexture> texture;
  SP<CFramebuffer> framebuffer;
  CBox originalGeometry;
  int workspaceId;
  int monitorId;
  bool isActive;
  CBox scaledGeometry;
  PHLANIMVAR<Vector2D> animPosition;
  PHLANIMVAR<Vector2D> animSize;
  bool needsUpdate;
  float lastCaptureTime;
  std::vector<CBox> pendingDamage;
  void *damageSubscription;
};

struct SWorkspaceSnapshot {
  PHLWORKSPACE workspace;
  int workspaceId;
  int monitorId;
  std::vector<SWindowSnapshot> windows;
  CBox layoutGeometry;
  bool needsLayout;
};

struct SMonitorOverviewState {
  int monitorId;
  eOverviewState state;
  std::vector<SWorkspaceSnapshot> workspaces;
  SWindowSnapshot *hoveredWindow;
  SWorkspaceSnapshot *centeredWorkspace;
  PHLANIMVAR<float> animProgress;
  float viewportOffset;
  float maxViewportOffset;
  float lastFrameTime;
  bool needsRedraw;

  struct {
    bool active;
    SWindowSnapshot *window;
    Vector2D startPos;
    Vector2D currentPos;
    int sourceMonitor;
  } dragState;

  struct {
    bool active;
    float startX;
  } scrollState;

  std::vector<SWindowSnapshot *> captureQueue;

  SMonitorOverviewState()
      : monitorId(-1), state(OVERVIEW_INACTIVE), hoveredWindow(nullptr),
        centeredWorkspace(nullptr), viewportOffset(0.0f),
        maxViewportOffset(0.0f), lastFrameTime(0.0f), needsRedraw(false) {
    dragState.active = false;
    scrollState.active = false;
  }
};

class CHyprview {
public:
  CHyprview();
  ~CHyprview();

  static CHyprview *get();

  void init(HANDLE handle);
  void shutdown();

  void toggle(int monitorId = -1);
  void activate(int monitorId);
  void deactivate(int monitorId);

  bool isActive(int monitorId = -1) const;
  eOverviewState getState(int monitorId) const;

  void onRenderWorkspace(void *thisptr, CMonitor *pMonitor,
                         PHLWORKSPACE pWorkspace, const CBox &box);
  bool onMouseButton(wlr_pointer_button_event *e);
  void onMouseMove(const Vector2D &pos);
  bool onKeyboardKey(wlr_keyboard_key_event *e);
  void onMonitorFrame(void *owner, void *data);
  void onWindowDamage(void *window, const CBox *box);
  void onWindowCommit(void *window);
  void onWindowDestroy(void *window);

  HANDLE getHandle() const { return m_phHandle; }
  const SHyprviewConfig &getConfig() const { return m_Config; }
  SMonitorOverviewState &getMonitorState(int monitorId);
  bool hasMonitorState(int monitorId) const {
    return m_MonitorStates.find(monitorId) != m_MonitorStates.end();
  }

private:
  void captureWorkspaces(int monitorId);
  void computeLayout(int monitorId);
  void renderOverview(CMonitor *pMonitor);
  void updateAnimation(SMonitorOverviewState &state, float deltaTime);
  void cleanupMonitorState(SMonitorOverviewState &state);
  void processCaptureQueue(SMonitorOverviewState &state);
  void captureWindowToFBO(SWindowSnapshot *pWindow, CMonitor *pMonitor);

  SWindowSnapshot *getWindowAtCursor(SMonitorOverviewState &state,
                                     const Vector2D &cursorPos);
  SWorkspaceSnapshot *getWorkspaceAtCursor(SMonitorOverviewState &state,
                                           const Vector2D &cursorPos);
  void moveWindowToWorkspace(SWindowSnapshot *pWindow,
                             SWorkspaceSnapshot *pTargetWorkspace);
  void scrollWorkspaces(SMonitorOverviewState &state, float deltaX);
  void navigateWindows(SMonitorOverviewState &state, int direction);
  void ensureWindowVisible(SMonitorOverviewState &state,
                           SWindowSnapshot *window);

  static float easeInOutCubic(float t);
  static float applyAnimationCurve(float t, const std::string &curve);

  static CHyprview *s_pInstance;

  HANDLE m_phHandle;
  SHyprviewConfig m_Config;
  std::unordered_map<int, SMonitorOverviewState> m_MonitorStates;

  CFunctionHook *m_pRenderWorkspaceHook;
  CFunctionHook *m_pMouseButtonHook;
  CFunctionHook *m_pMouseMoveHook;
  CFunctionHook *m_pKeyboardKeyHook;
  CFunctionHook *m_pMonitorFrameHook;
  CFunctionHook *m_pWindowDamageHook;
  CFunctionHook *m_pWindowCommitHook;
  CFunctionHook *m_pWindowDestroyHook;

  typedef void (*origRenderWorkspace)(void *, CMonitor *, PHLWORKSPACE,
                                      const CBox &);
  typedef void (*origMouseButton)(void *, wlr_pointer_button_event *);
  typedef void (*origMouseMove)(void *, const Vector2D &);
  typedef void (*origKeyboardKey)(void *, wlr_keyboard_key_event *);
  typedef void (*origMonitorFrame)(void *, void *);
  typedef void (*origWindowDamage)(void *, const CBox *);
  typedef void (*origWindowCommit)(void *);
  typedef void (*origWindowDestroy)(void *, PHLWINDOW);

  friend class CGestureManager;
  void beginGestureActivation(int monitorId);
  void updateGestureActivation(int monitorId, float progress);
  void commitGestureActivation(int monitorId, bool activate);
};

inline HANDLE PHANDLE = nullptr;
inline CHyprview *g_pHyprview = nullptr;

#endif
