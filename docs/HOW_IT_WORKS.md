# How Spaces-Plugins Works

This repository is the plugin hub for the main app:
- Main app: Spaces
- Plugin hub: Spaces-Plugins

## Architecture

Spaces uses a host-owned plugin runtime:
1. The host loads plugin manifests.
2. Plugins register capabilities.
3. The host renders UI and persists settings.
4. Plugins provide behavior and metadata, not host internals.

## Core Capability Model

- `commands`: command actions that can be invoked by host routes.
- `tray_contributions`: plugin commands visible in tray menus.
- `space_content_provider`: new Space content types.
- `appearance`: theme or visual mode related controls.
- `widgets`: widget-like Space surfaces.
- `desktop_context`: desktop context integrations.
- `settings_pages`: host-rendered settings pages.

## What Runs Where

Host repository responsibilities:
- Windowing and rendering.
- Persistence and settings storage.
- Plugin lifecycle and command dispatch.
- Security/validation policy.

Plugin hub responsibilities:
- Plugin examples and production-ready templates.
- Manifest standards and contracts.
- Contributor docs and release docs.
- Manifest/schema validation scripts.

## Settings and Persistence

Settings fields registered by plugins are persisted by the host.
Plugins should avoid writing custom global state unless required.

## Safety and Stability Rules

- Keep plugin behavior bounded to declared capability scope.
- Keep UI host-rendered and token-based.
- Avoid direct file operations outside plugin-managed or host-approved paths.
- Validate manifests before publishing.
