# Plugins Offered In This Repository

## Production / High Parity

- `Space-organizer`
- `folder-portal`
- `context-actions`
- `visual-modes`
- `sorting-clean-up`
- `rules-engine`
- `productivity-actions`
- `external-provider`

## MVP / Scaffold

- `widgets-plus`
- `clock-widget`
- `network-drive-Space`
- `powershell-Space`

## Plugin Folder Contract

Each plugin folder follows:

```text
plugins/
  <plugin-folder>/
    plugin.json
    README.md
    src/
      <PluginName>.h
      <PluginName>.cpp
```

## Manifest Contract

Every plugin should define:
- `id`
- `displayName`
- `version`
- `description`
- `author`
- `minHostVersion`
- `maxHostVersion`
- `minHostApiVersion`
- `maxHostApiVersion`
- `supportsSettingsPage`
- `supportsMainContentPage`
- `supportsHostedSummaryPanel`
- `icon` (optional)
- `updateChannelId` (optional)
- `hostedSummaryPanel`
- `capabilities`
- `repository`

## Validation

Use:
- `scripts/validate-plugin-manifests.ps1`
- `plugin-manifest.schema.json`
