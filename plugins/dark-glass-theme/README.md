# Dark Glass Theme

A translucent appearance plugin that provides a frosted-glass aesthetic for fence windows.

## Current state

This plugin is enabled by default and registers a complete appearance capability with 17+ persistent settings across two pages (Glass Style and Glass Behavior). The host can apply these styles to fence window rendering.

## Commands

- `dark_glass.reapply_theme`: reapplies the current Dark Glass settings immediately across fences.
- `dark_glass.preview_toggle`: flips `dark_glass.style.enabled` on/off and reapplies in one step for A/B testing.

## Quick validation path

- Open tray menu and choose **Reapply Dark Glass Theme** to validate changed settings without restarting the host.
- Open tray menu and choose **Toggle Dark Glass Preview** for one-click enabled/disabled comparisons.
- If `dark_glass.style.enabled` is false, reapply is intentionally skipped.

## What it does now

- registers the `appearance` capability with theme metadata
- declares two settings pages: **Glass Style** (core opacity, blur, borders, colors) and **Glass Behavior** (shadows, transparency, animations)
- provides stable settings keys for glass effect parameters
- supports dynamic adaptation based on battery status
- provides a command-triggered reapply path for rapid settings validation

## Key settings

**Baseline plugin settings:**

- `plugin.show_notifications`: emits notification events to diagnostics when enabled
- `plugin.refresh_interval_seconds`: throttles theme reapply refresh operations

**Glass Style page:**

- `enabled`: master toggle
- `opacity`, `blur_mode`: core glass effect
- `corner_radius`, `border_intensity`, `tint_hex`: aesthetic customization
- `show_border`, `header_opacity`: component-level controls
- `surface_variant`: themed surface choice

**Glass Behavior page:**

- `shadow_strength`: depth perception
- `compact_header`: space optimization
- `noise_overlay`: texture detail
- `active_contrast`, `inactive_blur`: state-aware rendering
- `reduce_transparency_on_battery`: power awareness
- `animate_focus_transition`: focus feedback

## What the host still needs

- shader or compositing backend for blur effect (Aero glass, Win32 Blur or DirectX)
- tint color application to window composition
- battery/power-state detection and automatic adaptation
- focus event callbacks to trigger transition animations

## Suggested host direction

Convert these settings into a Win32 Blur background effect (simplest) or DirectX-based glass shader (advanced). Consider:

- Preset themes: "Minimal Glass", "Dark Tint", "Frosted"
- Per-monitor DPI awareness for border and shadow scaling
- High contrast mode detection and fallback styling
