# Public Theme Package Contract

This document defines the stable contract for third-party theme packages consumed by IVOESimpleFences.

## Goals

- Keep one runtime theme authority: `win32_theme_system`.
- Let external authors add themes without host code changes.
- Keep host rendering safe: package provides tokens only.

## Required Fields

Each theme package manifest must provide:

- `id`: unique kebab-case theme id (example: `sunset-retro`).
- `displayName`: UI label only.
- `version`: semantic version (`major.minor.patch`).
- `source`: must be `win32_theme_system`.
- `tokens`: object map of token names to token values.

## Optional Metadata

- `author`
- `website`
- `previewImage`
- `description`

## Security and Validation Rules

- Package must not contain executables or scripts.
- Token keys must match approved namespace and naming format.
- Required token families must be present before apply.
- Invalid packages are rejected with diagnostics.

## Host Settings Bridge Keys

Host persistence normalizes and keeps these keys:

- `theme.source`
- `theme.win32.theme_id`
- `theme.win32.display_name`
- `theme.win32.catalog_version`

## Compatibility Notes

- Unknown theme ids fall back to `graphite-office`.
- `displayName` is not a canonical identifier.
- Legacy underscore ids are migrated to kebab-case once.

## Samples

- Schema: `theme-package.schema.json`
- Valid sample: `samples/theme-package.sample.json`
- Settings sample: `samples/host-theme-settings.sample.json`
