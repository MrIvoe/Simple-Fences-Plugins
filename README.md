# Simple Fences Plugin Hub

Community plugins, examples, and starter templates for [IVOESimpleFences](https://github.com/MrIvoe/IVOESimpleFences).

## Current phase

This hub currently targets the host's platform-foundation phase. On the main app side, the plugin model is real but still early:

- manifests, plugin lifecycle, settings pages, and content-provider registration exist now
- settings persistence exists now
- several advanced experiences are still scaffold-level in the host, including richer widget surfaces and non-file content rendering

That means this repository should do two things well:

1. provide API-aligned sample plugins that compile cleanly against the host
2. make it easy for contributors to copy a template and extend the platform safely

For production-oriented plugin planning, see [PLUGIN_CATALOG.md](PLUGIN_CATALOG.md).

## Why this repo exists

SimpleFences is a lightweight Win32 desktop organizer with real fence windows and real backing folders. This repo is the community layer around that app.

The goal is not just "more plugins." The goal is to move the project closer to a strong Stardock Fences-style experience while keeping the code understandable and the file-handling model safe.

High-value plugin directions include:

- richer content providers such as network, portal, script, and workspace fences
- more polished appearance packs and themes
- widget panels that surface useful live information inside a fence
- settings pages that let non-developers customize plugins without editing files by hand

## Capability overview

| Capability | What it unlocks |
| --- | --- |
| `commands` | Register command-dispatch actions |
| `tray_contributions` | Add items to the tray menu |
| `fence_content_provider` | Register a fence content type such as a network path or scripted workspace |
| `appearance` | Provide appearance/theme controls for fence windows |
| `widgets` | Add embeddable widget panels inside a fence |
| `desktop_context` | Extend desktop context behavior |
| `settings_pages` | Register persisted settings pages with checkbox, text, number, and enum fields |

All fields registered through `settings_pages` are persisted automatically to `%LOCALAPPDATA%\SimpleFences\settings.json`.

## Shared plugin UI pattern

Plugin settings UI in this repository is host-rendered through `SettingsWindow` in the main app. To keep plugin pages visually cohesive and maintainable, plugins now use a shared helper:

- `plugins/shared/PluginUiPatterns.h`

Current baseline convention:

- `plugin.show_notifications`
- `plugin.refresh_interval_seconds`

Use `PluginUiPatterns::AppendBaselineSettingsFields(...)` at the top of each settings page field list so labels, ordering, and descriptions stay consistent across plugins.

## Included plugins

| Plugin | Capabilities | Status | What it adds |
| --- | --- | --- | --- |
| [plugins/clock-widget](plugins/clock-widget) | `widgets`, `settings_pages` | Sample | A configurable clock widget with display and behavior settings |
| [plugins/dark-glass-theme](plugins/dark-glass-theme) | `appearance`, `settings_pages` | Sample | A translucent dark-glass theme with more tunable style controls |
| [plugins/network-drive-fence](plugins/network-drive-fence) | `fence_content_provider`, `settings_pages` | Sample | A network path fence provider with reconnect and offline behavior settings |
| [plugins/powershell-fence](plugins/powershell-fence) | `fence_content_provider`, `settings_pages` | Prototype | A PowerShell workspace fence concept with startup, view, and security settings |
| [plugins/fence-organizer](plugins/fence-organizer) | `commands`, `menu_contributions`, `settings_pages` | Production | Sorting, grouping, and file analysis commands for organizing fence contents |
| [plugins/folder-portal](plugins/folder-portal) | `fence_content_provider`, `commands`, `menu_contributions`, `tray_contributions`, `settings_pages` | MVP | Existing-folder-backed portal fences with health-aware settings and actions |
| [plugins/context-actions](plugins/context-actions) | `commands`, `menu_contributions`, `desktop_context`, `settings_pages` | MVP | Contextual desktop/fence/item action contributions with settings-driven visibility |
| [plugins/visual-modes](plugins/visual-modes) | `appearance`, `commands`, `tray_contributions`, `settings_pages` | Production | Global and per-fence visual preset commands with persisted appearance settings |
| [plugins/sorting-clean-up](plugins/sorting-clean-up) | `commands`, `tray_contributions`, `settings_pages`, `desktop_context` | Production | Sorting, cleanup, and autosort command set with persisted behavior controls |
| [plugins/rules-engine](plugins/rules-engine) | `commands`, `settings_pages`, `tray_contributions` | Production | Ordered rule evaluation commands with dry-run, pause, and export controls |
| [plugins/productivity-actions](plugins/productivity-actions) | `commands`, `tray_contributions`, `settings_pages`, `desktop_context` | Production | Multi-step project, archive, rename, open, and snapshot fence actions |
| [plugins/widgets-plus](plugins/widgets-plus) | `widgets`, `commands`, `settings_pages`, `tray_contributions` | MVP | Utility widget command set with persisted refresh and pause behavior |
| [plugins/external-provider](plugins/external-provider) | `fence_content_provider`, `commands`, `settings_pages`, `tray_contributions` | MVP | External and generated provider-backed fence content with cache and reconnect flows |

## Host API parity matrix

Legend:

- Behavior-complete: sample demonstrates real behavior on current host contracts
- Scaffold-only: sample is contract-aligned but intentionally placeholder-oriented

| Plugin | Host API parity |
| --- | --- |
| [plugins/fence-organizer](plugins/fence-organizer) | Behavior-complete |
| [plugins/folder-portal](plugins/folder-portal) | Behavior-complete |
| [plugins/context-actions](plugins/context-actions) | Behavior-complete |
| [plugins/visual-modes](plugins/visual-modes) | Behavior-complete |
| [plugins/sorting-clean-up](plugins/sorting-clean-up) | Behavior-complete |
| [plugins/rules-engine](plugins/rules-engine) | Behavior-complete |
| [plugins/productivity-actions](plugins/productivity-actions) | Behavior-complete |
| [plugins/widgets-plus](plugins/widgets-plus) | Scaffold-only |
| [plugins/external-provider](plugins/external-provider) | Behavior-complete |
| [plugins/clock-widget](plugins/clock-widget) | Scaffold-only |
| [plugins/dark-glass-theme](plugins/dark-glass-theme) | Scaffold-only |
| [plugins/network-drive-fence](plugins/network-drive-fence) | Scaffold-only |
| [plugins/powershell-fence](plugins/powershell-fence) | Scaffold-only |

## PowerShell fence feasibility

Yes, a PowerShell-based fence is a sensible direction, but there is an important platform boundary.

What can be done in this repo today:

- define a plugin manifest and lifecycle implementation
- register a PowerShell-oriented content type with the host
- define persisted settings for startup scripts, working directory, execution behavior, and safety controls
- document the host-side work needed for a real embedded terminal surface

What still needs host support in [IVOESimpleFences](https://github.com/MrIvoe/IVOESimpleFences):

- a renderable terminal or hosted child-window surface inside a fence
- provider callbacks that can materialize PowerShell content rather than only register provider metadata
- lifecycle hooks for process/session start, reuse, shutdown, and focus routing

This repo now includes a PowerShell fence prototype so the configuration contract is ready when the host grows that surface.

## Quick start for plugin authors

1. Copy [plugin-template](plugin-template) to a new folder under `plugins/<your-kebab-name>/`.
2. Rename the source files and replace the template plugin ids/classes.
3. Update `plugin.json`.
4. Copy the plugin into your local SimpleFences host checkout.
5. Add the `.cpp` and `.h` files to the host `CMakeLists.txt` and register the plugin in `BuiltinPlugins.cpp`.
6. Build the host and verify settings persistence.

Registration example:

```cpp
#include "plugins/community/clock_widget/ClockWidgetPlugin.h"

plugins.push_back(std::make_unique<ClockWidgetPlugin>());
```

## Building against the host

This repository is a plugin hub, not a standalone executable. To compile a plugin, build it inside the main app repository.

```powershell
# From the IVOESimpleFences repo root
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
.\build\bin\Debug\SimpleFences.exe
```

Recommended manual verification after adding or changing a plugin:

1. Confirm `SimpleFences.exe` is not already running.
2. Build Debug.
3. Launch the app.
4. Open Settings and find the plugin page.
5. Change several settings.
6. Restart the app and verify those settings persisted.

## Repository layout

```text
plugins/
    <plugin-folder>/
        plugin.json
        README.md                optional but recommended
        src/
            <PluginName>.h
            <PluginName>.cpp

plugin-template/
    plugin.json
    README.md
    src/
        TemplatePlugin.h
        TemplatePlugin.cpp
```

## Naming conventions

| Thing | Convention |
| --- | --- |
| Plugin id | `community.<snake_case>` |
| Field storage key | `<plugin_short_name>.<page>.<field>` |
| Source files | `PascalCasePlugin.h` and `.cpp` |
| Hub folder | `plugins/<kebab-case>/` |
| Content type | a stable noun such as `network_drive` or `powershell_workspace` |
| Provider id | the concrete implementation identity, usually matching the manifest id |

## Local validation

The repository includes manifest validation in GitHub Actions for every `plugins/**/plugin.json` file. Keep template assets outside the `plugins/` folder so the template stays copyable without failing validation.

Before opening a pull request:

1. validate every changed `plugin.json`
2. build the host repo with your plugin added
3. verify settings persistence after restart
4. update the relevant README entries if behavior changed

## Contributing

Use [PLUGIN_GUIDE.md](PLUGIN_GUIDE.md) for the implementation walkthrough and [CONTRIBUTING.md](CONTRIBUTING.md) for the pull-request checklist.

If your plugin needs a host capability that does not exist yet, open an issue in the main [IVOESimpleFences](https://github.com/MrIvoe/IVOESimpleFences) repo and describe the missing contract clearly.

## License

MIT. See [LICENSE](LICENSE). Plugin authors retain ownership of their own plugin code.
