# Simple Spaces Plugin Hub

[![Validate plugin manifests](https://github.com/MrIvoe/Spaces-Plugins/actions/workflows/validate-plugins.yml/badge.svg)](https://github.com/MrIvoe/Spaces-Plugins/actions/workflows/validate-plugins.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Repo stars](https://img.shields.io/github/stars/MrIvoe/Spaces-Plugins?style=social)](https://github.com/MrIvoe/Spaces-Plugins/stargazers)

Spaces-Plugins is the official plugin hub for Spaces.

This repository provides:
- production-ready and sample plugins
- plugin template and manifest schema
- contributor and release documentation
- host integration references

## What The Program Offers

Spaces + this plugin hub gives you:
- plugin-based desktop Space extensions
- content provider plugins (portal/external/network/scripted)
- settings-page contributions with persisted values
- tray and desktop-context command extensions
- appearance and widget extension points

## Documentation Hub

Start here:
- [Documentation Index](docs/INDEX.md)
- [Docs Sidebar](docs/SIDEBAR.md)
- [How It Works](docs/HOW_IT_WORKS.md)
- [Plugins Offered](docs/PLUGINS.md)
- [Create A Plugin](docs/CREATE_A_PLUGIN.md)
- [Release Guide](docs/RELEASE.md)
- [Host Integration](docs/HOST_INTEGRATION.md)
- [Changelog](docs/CHANGELOG.md)
- [Unused File Audit](docs/UNUSED_FILE_AUDIT.md)

## Repository Layout

```text
plugin-template/
plugins/
docs/
scripts/
plugin-manifest.schema.json
```

## Quick Start

1. Copy `plugin-template/` to `plugins/<your-plugin>/`.
2. Update `plugin.json` and source files.
3. Validate manifests with `scripts/validate-plugin-manifests.ps1`.
4. Integrate into Spaces and test startup/settings persistence.

## Validation

Use:

```powershell
./scripts/validate-plugin-manifests.ps1
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

See [LICENSE](LICENSE).
