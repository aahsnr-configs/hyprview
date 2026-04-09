# README.md

# hyprview

A Hyprland plugin that brings a **Niri‑style scrollable overview** to Hyprland.  
Zoom out to see all workspaces in a grid, drag windows between them, and navigate smoothly with mouse, keyboard, or touchpad gestures.

![Screenshot](screenshots/overview.png) _(add your own screenshot)_

## Features

- **Niri‑style horizontal workspace scrolling** – Right‑click drag or two‑finger swipe to pan across workspaces
- **Live window thumbnails** – Real‑time previews using FBO capture (with automatic updates)
- **Drag and drop** – Move windows between workspaces by dragging their thumbnails
- **Keyboard navigation** – Vim‑style (`hjkl`) or arrow keys to focus windows, `Enter` to select, `Tab` to cycle workspaces
- **Multi‑monitor support** – Independent overview on each display
- **Smooth animations** – Configurable easing curves (linear / easeInOutCubic)
- **Touchpad gestures** – 4‑finger swipe to open/close, 2‑finger scroll to pan
- **Highly configurable** – Colors, gaps, animation duration, zoom level, and more

## Installation

### Via hyprpm (recommended)

```bash
hyprpm add https://github.com/yourusername/hyprview
hyprpm enable hyprview
```

Then restart Hyprland or reload plugins with `hyprpm reload`.

### Manual Build

```bash
git clone https://github.com/yourusername/hyprview
cd hyprview
make
mkdir -p ~/.local/share/hyprland/plugins
cp hyprview.so ~/.local/share/hyprland/plugins/
```

Then add to your `~/.config/hypr/hyprland.conf`:

```ini
plugin = ~/.local/share/hyprland/plugins/hyprview.so
```

## Configuration

All settings are placed inside the `plugin` block in your Hyprland config.

```ini
plugin {
    hyprview {
        # Layout
        columns = 3                      # Number of workspace columns
        gap_size = 5                     # Gap between window thumbnails (px)
        border_size = 2                  # Thickness of window borders (px)
        border_radius = 4                # Corner radius for windows and workspaces
        workspace_spacing = 40           # Horizontal spacing between workspaces (px)
        zoom_level = 0.7                 # Scale factor for window thumbnails
        skip_empty = true                # Hide workspaces with no windows
        layout_mode = grid               # Layout algorithm (currently only "grid")

        # Colors (use rgba hex or named colors)
        bg_color = rgba(000000ff)        # Background overlay color
        dim_opacity = 0.6                # Dimming intensity (0.0 - 1.0)
        border_color_active = rgba(5294e2ff)   # Border for focused window
        border_color_inactive = rgba(7a7a7aff) # Border for unfocused windows
        hover_border_color = rgba(ffffffff)    # Border when mouse hovers
        workspace_label_color = rgba(ffffffff) # Workspace name text color

        # Animation
        animation_duration = 0.25        # Transition time in seconds
        animation_curve = easeInOutCubic # "linear" or "easeInOutCubic"
        hover_scale = 1.02               # Slight enlargement on hover

        # Gestures
        gestures_enabled = true          # Enable touchpad gestures
        gesture_fingers = 4              # Number of fingers for swipe
        gesture_distance = 300           # Pixels needed to trigger toggle

        # Performance
        max_fps_capture = 30             # Max FPS for window thumbnail updates
        max_captures_per_frame = 2       # Thumbnails captured per frame

        # Behavior
        close_on_click_empty = true      # Close overview when clicking empty space
        show_workspace_labels = true     # Display workspace names above containers
        workspace_label_font_size = 14   # Font size for workspace labels
    }
}
```

## Keybinds

Add these to your `hyprland.conf`:

```ini
bind = SUPER, G, hyprview:toggle   # Toggle overview on current monitor
# bind = SUPER, O, hyprview:open   # Force open
# bind = SUPER, C, hyprview:close  # Force close
```

## Usage

| Action                           | Mouse / Touchpad                                | Keyboard                      |
| -------------------------------- | ----------------------------------------------- | ----------------------------- |
| Open/Close overview              | 4‑finger swipe                                  | `SUPER + G` (configurable)    |
| Navigate workspaces              | Right‑click drag horizontally / 2‑finger scroll | `Tab` / `PageUp/PageDown`     |
| Move window to another workspace | Left‑click drag window thumbnail                | –                             |
| Focus a window                   | Click on thumbnail                              | Arrow keys / `hjkl` + `Enter` |
| Exit overview                    | Click empty space / `ESC`                       | `ESC`                         |

## Building from Source

```bash
git clone https://github.com/yourusername/hyprview
cd hyprview
./build.sh      # or simply 'make'
```

## Troubleshooting

| Issue                                   | Solution                                                                   |
| --------------------------------------- | -------------------------------------------------------------------------- |
| Plugin fails to load (version mismatch) | Ensure Hyprland headers match installed version. Rebuild plugin locally.   |
| No thumbnails, only colored rectangles  | Check that `max_fps_capture` is not 0 and that FBO allocation succeeded.   |
| Gestures not working                    | Verify `gestures_enabled = true` and that your touchpad supports gestures. |
| Crash on activation                     | Run Hyprland with `HYPRLAND_LOG_LEVEL=3` and check logs in `/tmp/hypr/`.   |

## License

MIT License – see [LICENSE](LICENSE) file.

## Acknowledgments

Inspired by [Niri](https://github.com/YaLTeR/niri) and the official Hyprland [`hyprexpo`](https://github.com/hyprwm/hyprland-plugins) plugin.
