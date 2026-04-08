# Plugin Template

Copy this folder into `plugins/<your-plugin-name>/` and then rename the identifiers.

## Rename checklist

1. Rename `TemplatePlugin.h` and `TemplatePlugin.cpp`.
2. Rename the class from `TemplatePlugin` to your plugin class.
3. Replace `community.template_plugin` with your real manifest id.
4. Replace `template.general` and related field keys.
5. Update `plugin.json`.
6. Add the plugin to the host build and `BuiltinPlugins.cpp`.

## What this template includes

- a valid manifest
- a minimal plugin class
- one settings page with baseline plugin UI fields plus example boolean and enum fields
- naming and structure that match the rest of the hub

## Shared UI baseline

The template uses `PluginUiPatterns::AppendBaselineSettingsFields(...)` so new plugins start with consistent host-rendered settings UX:

- `plugin.show_notifications`
- `plugin.refresh_interval_seconds`

The shared baseline also includes standardized UI policy keys under `plugin.ui.*` so new plugins default to:

- host resources first for colors/icons
- standard margins/padding
- standard section headers/cards

## Standard manifest checklist

Keep these fields in your copied `plugin.json`:

- `id`, `displayName`, `version`, `description`, `author`
- `minHostVersion`, `maxHostVersion`
- `minHostApiVersion`, `maxHostApiVersion`
- `supportsSettingsPage`, `supportsMainContentPage`
- optional `icon`, optional `updateChannelId`
- `capabilities`, `repository`

## Update model

Ship packaged plugin artifacts and publish update metadata/manifest JSON. Do not rely on the main app reading plugin repository folders directly.

## Recommended next step

If your plugin adds a new Space content type, follow [docs/CREATE_A_PLUGIN.md](../docs/CREATE_A_PLUGIN.md) and register both `contentType` and `providerId` explicitly.
