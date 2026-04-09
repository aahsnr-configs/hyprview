#include "Hyprview.hpp"

inline HANDLE PHANDLE = nullptr;
inline CHyprview *g_pHyprview = nullptr;

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  const std::string HASH = __hyprland_api_get_hash();
  if (HASH != GIT_COMMIT_HASH) {
    HyprlandAPI::addNotification(PHANDLE, "[hyprview] Version mismatch!",
                                 CColor{1.0, 0.2, 0.2, 1.0}, 5000);
    throw std::runtime_error("[hyprview] Version mismatch");
  }

  g_pHyprview = new CHyprview();
  g_pHyprview->init(handle);

  return {"hyprview", "A Hyprland overview plugin with Niri-style scrolling",
          "Hyprland Community", "0.2.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
  if (g_pHyprview) {
    g_pHyprview->shutdown();
    delete g_pHyprview;
    g_pHyprview = nullptr;
  }
}
