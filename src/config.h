#ifndef HYPRVIEW_CONFIG_H
#define HYPRVIEW_CONFIG_H

#include <hyprland/src/defines.hpp>
#include <hyprland/src/helpers/Color.hpp>

struct SHyprviewConfig {
  // Layout
  int columns = 3;
  int gap_size = 5;
  int border_size = 2;
  int border_radius = 4;
  int workspace_spacing = 40;
  float zoom_level = 0.7f;
  bool skip_empty = true;
  std::string layout_mode = "grid";

  // Colors
  CColor bg_color = {0.0f, 0.0f, 0.0f, 1.0f};
  float dim_opacity = 0.6f;
  CColor border_color_active = {0.2f, 0.5f, 0.9f, 1.0f};
  CColor border_color_inactive = {0.5f, 0.5f, 0.5f, 1.0f};
  CColor hover_border_color = {1.0f, 1.0f, 1.0f, 1.0f};
  CColor workspace_label_color = {1.0f, 1.0f, 1.0f, 1.0f};

  // Animation
  float animation_duration = 0.25f;
  std::string animation_curve = "easeInOutCubic";
  float hover_scale = 1.02f;

  // Gestures
  bool gestures_enabled = true;
  int gesture_fingers = 4;
  int gesture_distance = 300;

  // Performance
  int max_fps_capture = 30;
  int max_captures_per_frame = 2;

  // Behavior
  bool close_on_click_empty = true;
  bool show_workspace_labels = true;
  int workspace_label_font_size = 14;
};

#endif
