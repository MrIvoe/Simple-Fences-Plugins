# Folder Portal

Create Spaces backed by existing folders with provider callbacks and health-aware state updates.

## Plugin Snapshot

- Folder: `folder-portal`
- Plugin ID: `community.folder_portal`
- Version: `1.2.2`
- Capabilities: `space_content_provider`, `commands`, `menu_contributions`, `tray_contributions`, `settings_pages`

## Files

- `plugin.json`
- `src/FolderPortalPlugin.h`
- `src/FolderPortalPlugin.cpp`

## Host Integration

1. Copy this plugin folder into the host plugin source location.
2. Register plugin source in host build configuration.
3. Register plugin in host plugin bootstrap (`BuiltinPlugins.cpp`).
4. Build and run host app.

## Validation

1. Run manifest validator from this repo root:
   - `./scripts/validate-plugin-manifests.ps1`
2. In host app, verify:
   - settings page visibility (if applicable)
   - command/menu behavior (if applicable)
   - startup stability and settings persistence

## Related Docs

- [Create A Plugin](../../docs/CREATE_A_PLUGIN.md)
- [How It Works](../../docs/HOW_IT_WORKS.md)
- [Plugins Offered](../../docs/PLUGINS.md)
- [Release Guide](../../docs/RELEASE.md)
