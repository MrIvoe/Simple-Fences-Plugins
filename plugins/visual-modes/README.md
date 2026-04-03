# Visual Modes

Visual Modes is a Tier 1 catalog sample that applies global or per-fence visual behavior presets through plugin commands and persisted settings.

This plugin now uses the Win32ThemeSystem catalog naming model so the same theme family names can be surfaced consistently across host integrations.

## Capabilities

- appearance
- commands
- tray_contributions
- settings_pages

## Commands

- theme.switch
- theme.compact_toggle
- theme.presentation_toggle
- theme.host_bridge_sync
- theme.apply_current_to_fence
- theme.reset_fence
- appearance.mode.focus (compat alias -> apply current preset to target fence)
- appearance.mode.cycle (compat alias -> switch preset)
- appearance.mode.reset (compat alias -> reset target fence)

## Settings

- plugin.enabled
- plugin.log_actions
- plugin.show_notifications
- plugin.safe_mode
- plugin.default_mode
- plugin.config_source
- plugin.refresh_interval_seconds
- theme.preset
- theme.apply_global
- theme.allow_per_fence_override
- theme.source
- theme.win32.display_name
- theme.win32.catalog_version
- theme.colors.background
- theme.colors.header
- theme.colors.border
- theme.colors.text
- theme.effects.transparency
- theme.keep_title_bar_visible
- theme.effects.opacity_percent
- theme.effects.blur
- theme.effects.corner_radius_px

## Behavior summary

- Switch command cycles preset values and persists the active preset.
- Compact toggle switches between Mono Minimal and Graphite Office.
- Presentation toggle switches between Storm Steel and Aurora Light.
- Apply/reset commands target a single fence when command context includes fence payload.
- When global apply is enabled, preset switches can apply to all fences.
- Rollup + transparency combinations are clamped to keep the title bar visible by default.

## Win32ThemeSystem families

- Amber Terminal
- Arctic Glass
- Aurora Light
- Brass Steampunk
- Copper Foundry
- Emerald Ledger
- Forest Organic
- Graphite Office
- Harbor Blue
- Ivory Bureau
- Mono Minimal
- Neon Cyberpunk
- Nocturne Dark
- Nova Futuristic
- Olive Terminal
- Pop Colorburst
- Rose Paper
- Storm Steel
- Sunset Retro
- Tape Lo-Fi

## Host bridge contract

Visual Modes now emits normalized bridge settings that a host can consume to call Win32ThemeSystem APIs directly:

- `theme.preset`: stable key used by plugin commands
- `theme.win32.display_name`: mapped Win32ThemeSystem display name (for example `Graphite Office`)
- `theme.source`: currently `win32_theme_system`
- `theme.win32.catalog_version`: emitted catalog contract version

`theme.host_bridge_sync` rewrites these keys from the current preset so host integrations can re-sync without changing selection.

## Host API usage

This sample demonstrates:

- routed command payload consumption via CommandContext
- persisted settings updates via PluginSettingsRegistry
- tray command contributions via MenuContributionRegistry
- visual behavior updates via IApplicationCommands::UpdateFencePresentation

## Notes

- Color and blur values are persisted and ready for host renderer consumption.
- The sample maps presets to FencePresentationSettings so behavior can be tested even before full compositor support is present.

## Host registration snippet

```cpp
#include "plugins/community/visual_modes/VisualModesPlugin.h"

plugins.push_back(std::make_unique<VisualModesPlugin>());
```
