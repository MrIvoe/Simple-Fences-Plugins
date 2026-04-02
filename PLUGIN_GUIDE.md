# Plugin Development Guide

This guide explains how to build a SimpleFences plugin that is aligned with the current host API and easy for other contributors to extend.

## Current phase

The host is still in its plugin-foundation phase. Treat this as a real integration surface, but not yet a fully mature extension SDK.

Reliable surfaces today:

- plugin manifests and lifecycle
- settings page registration and persistence
- content-provider registration metadata

Surfaces still evolving in the host:

- richer widget rendering contracts
- live provider callbacks for non-file-backed fence content
- plugin management UX and failure reporting

Build your plugin so it works cleanly with the current contracts and degrades safely when a future host feature is not present yet.

## Minimum requirements

- Windows 10 or later
- Visual Studio 2022 or Build Tools with Desktop C++
- CMake 3.16+
- Local checkout of [IVOESimpleFences](https://github.com/MrIvoe/IVOESimpleFences)
- C++17

## Start from the template

The fastest path is to copy [plugin-template](plugin-template) into a new folder under `plugins/` and rename the files and identifiers.

That gives you:

- a valid `plugin.json`
- a minimal `IPlugin` implementation
- a settings page example
- naming patterns that match the rest of the hub

## 1. Create `plugin.json`

Every plugin ships a manifest.

```json
{
  "id": "community.my_awesome_plugin",
  "displayName": "My Awesome Plugin",
  "version": "1.0.0",
  "description": "Short one-line description visible in the Settings Hub.",
  "author": "Your Name",
  "minHostVersion": "0.0.009",
  "capabilities": ["settings_pages"],
  "repository": "https://github.com/MrIvoe/Simple-Fences-Plugins"
}
```

### Required keys

| Key | Type | Description |
| --- | --- | --- |
| `id` | string | Unique id. Community plugins should use the `community.` prefix. |
| `displayName` | string | Name shown in plugin listings and settings navigation. |
| `version` | string | Semantic version string. |
| `description` | string | Short summary of the plugin. |
| `author` | string | Plugin author or organization. |
| `minHostVersion` | string | Minimum host version your plugin expects. Keep this aligned with the host branch you tested. |
| `capabilities` | array | One or more capability strings. |

### Capability choices

Choose only the capabilities you actually implement.

| Capability | Use it when your plugin... |
| --- | --- |
| `settings_pages` | registers UI settings fields |
| `fence_content_provider` | introduces a new fence content type |
| `appearance` | influences fence styling or theme choices |
| `widgets` | adds fence-embedded widget behavior |
| `commands` | exposes command-dispatch actions |
| `tray_contributions` | contributes tray items |
| `desktop_context` | extends desktop-context behavior |

## 2. Implement `IPlugin`

Header:

```cpp
#pragma once

#include "extensions/PluginContracts.h"

class MyPlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;
};
```

Implementation:

```cpp
#include "MyPlugin.h"

#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"

PluginManifest MyPlugin::GetManifest() const
{
    PluginManifest m;
    m.id           = L"community.my_awesome_plugin";
    m.displayName  = L"My Awesome Plugin";
    m.version      = L"1.0.0";
    m.description  = L"Short one-line description visible in the Settings Hub.";
    m.capabilities = {L"settings_pages"};
    return m;
}

bool MyPlugin::Initialize(const PluginContext& context)
{
    if (context.settingsRegistry)
    {
        PluginSettingsPage page;
        page.pluginId = L"community.my_awesome_plugin";
        page.pageId   = L"my_plugin.general";
        page.title    = L"General";
        page.order    = 10;

        page.fields.push_back(SettingsFieldDescriptor{
            L"my_plugin.general.enabled",
            L"Enable plugin",
            L"Turns the plugin behavior on or off.",
            SettingsFieldType::Bool, L"true", {}, 10
        });

        page.fields.push_back(SettingsFieldDescriptor{
            L"my_plugin.general.mode",
            L"Mode",
            L"Select the operating mode for the plugin.",
            SettingsFieldType::Enum, L"safe",
            {
                {L"safe",  L"Safe"},
                {L"fast",  L"Fast"},
                {L"debug", L"Debug"},
            },
            20
        });

        context.settingsRegistry->RegisterPage(std::move(page));
    }

    return true;
}

void MyPlugin::Shutdown() {}
```

## 3. Register content providers correctly

If your plugin adds a new fence content type, use the current host descriptor shape:

```cpp
#include "extensions/FenceExtensionRegistry.h"

if (context.fenceExtensionRegistry)
{
    FenceContentProviderDescriptor descriptor;
    descriptor.providerId   = L"community.my_awesome_plugin";
    descriptor.contentType  = L"my_content_type";
    descriptor.displayName  = L"My Content Type";
    descriptor.isCoreDefault = false;

    context.fenceExtensionRegistry->RegisterContentProvider(descriptor);
}
```

Use these fields deliberately:

- `contentType`: the stable saved fence content classification
- `providerId`: the concrete implementation that serves that content type
- `displayName`: what users see in UI
- `isCoreDefault`: only `true` when your provider is the default implementation for that content type

## 4. Register the plugin with the host

In the host repo, add your plugin source files to the build and register the plugin in `BuiltinPlugins.cpp`.

```cpp
#include "plugins/community/my_awesome_plugin/MyPlugin.h"

plugins.push_back(std::make_unique<MyPlugin>());
```

## 5. Supported field types

| `SettingsFieldType` | Control rendered | `defaultValue` format |
| --- | --- | --- |
| `Bool` | Checkbox | `"true"` or `"false"` |
| `Int` | Numeric edit box | Decimal integer string such as `"60"` |
| `String` | Text edit box | Any string |
| `Enum` | Drop-down | One of the declared option values |

Values are persisted automatically to `%LOCALAPPDATA%\SimpleFences\settings.json`.

## 6. Naming conventions

| Thing | Convention |
| --- | --- |
| Plugin id | `community.<snake_case>` |
| Field key | `<plugin_short_name>.<page>.<field>` |
| Content type | short, stable noun such as `network_drive` |
| Provider id | usually the manifest id |
| Source files | `PascalCasePlugin.h` and `.cpp` |
| Plugin folder | `plugins/<kebab-case-plugin-name>/` |

## 7. Design for good settings pages

A strong plugin usually has more than one setting that matters in practice. Useful settings tend to fall into a few buckets:

- behavior toggles
- display and visual tuning
- refresh or polling intervals
- default paths or startup targets
- safety controls for anything that launches processes or touches the filesystem

Do not add settings just to look configurable. Every field should map to a clear runtime decision.

## 8. Manual verification checklist

Run these checks against the main host repo after copying in your plugin:

1. Confirm `SimpleFences.exe` is not already running.
2. Build Debug.
3. Launch the app.
4. Open Settings and locate the plugin page.
5. Change multiple fields, including enum and text values.
6. Restart the app.
7. Confirm those values persisted.
8. If the plugin adds a fence content type, verify that the new type is discoverable and saved correctly.

Build commands:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
.\build\bin\Debug\SimpleFences.exe
```

## 9. Current platform limits to keep in mind

At the moment, this hub can define real contracts and realistic samples, but some advanced plugin behaviors still require host-side growth.

Examples:

- a PowerShell fence needs an embeddable terminal surface in the host
- richer widgets need a stronger rendering contract than simple metadata registration
- portal-style providers need non-file content materialization hooks

If your idea depends on one of those gaps, submit the plugin scaffold here and open a matching host issue so the contract work is tracked explicitly.
