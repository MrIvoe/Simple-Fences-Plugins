# Theme Integration

Theme integration for plugins must align with host-owned rendering and validated token contracts.

References:

- docs/THEME_PACKAGE_CONTRACT.md
- docs/HOST_INTEGRATION.md

Rules:

- plugin/theme payloads provide data, not direct compositor writes
- invalid theme payloads must be rejected with diagnostics
- host canonical theme keys and source remain authoritative
- plugins should resolve semantic paths to token values instead of hardcoding color literals

Implementation helper:

- plugins/shared/ThemeTokenHelper.h provides semantic -> token -> color lookup for plugin code paths
- plugin code should prefer semantic role requests (for example plugin.header.bg) rather than direct palette values
