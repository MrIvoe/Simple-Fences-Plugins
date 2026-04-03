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

## Recommended next step

If your plugin adds a new fence content type, follow the content-provider example in [PLUGIN_GUIDE.md](../PLUGIN_GUIDE.md) and register both `contentType` and `providerId` explicitly.
