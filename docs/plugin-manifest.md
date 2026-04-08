# Plugin Manifest

## Required Fields

- id
- displayName
- version
- author
- description
- host compatibility range (legacy min/max keys or `compatibility` object)
- capabilities
- settings metadata

## Compatibility Model

Preferred model:

- `compatibility.hostVersion.min`
- `compatibility.hostVersion.max`
- `compatibility.hostApiVersion.min`
- `compatibility.hostApiVersion.max`

Backward compatibility:

- Legacy keys (`minHostVersion`, `maxHostVersion`, `minHostApiVersion`, `maxHostApiVersion`) are still accepted.
- If both formats are present, values must match.

## Permissions Declaration

Optional `permissions` array can be used to declare sensitive capability intent.

Current allowed permissions:

- `read_space_metadata`
- `write_space_settings`
- `read_settings`
- `write_settings`
- `network_access`
- `filesystem_read`
- `filesystem_write`
- `execute_host_commands`

## Validation

Manifest validity should be enforced before host load.

Existing references:

- plugin-template/plugin.json
- docs/THEME_PACKAGE_CONTRACT.md
- scripts/validate-plugin-manifests.ps1
