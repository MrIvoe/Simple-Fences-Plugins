# Cross-Repo Theme Contract

Status: active
Contract version: 1.0.0
Updated: 2026-04-08

## Purpose

Define the public contract between Themes, Spaces, and Spaces-Plugins for token bundles, semantic mappings, compatibility, and fallback behavior.

## Source Of Truth

- Themes is the canonical source for token schema and semantic mapping schema.
- Spaces exposes host theme capabilities and applies platform adapters.
- Spaces-Plugins consume host-provided semantic tokens and do not write raw palette values.

## Canonical Namespaces

- `mrivoe.theme`
- `mrivoe.theme.tokens`
- `mrivoe.theme.semantic`
- `mrivoe.theme.win32` (adapter-specific surface)

## Plugin Contract Expectations

- Plugin manifests must use supported theme token namespace values.
- Plugin UIs should bind to semantic tokens, not hardcoded palette literals.
- Plugin fallbacks must defer to host defaults when semantic keys are unavailable.

## Compatibility Rules

- Major contract changes require a contract version bump.
- Minor changes may add optional semantic keys.
- Patch changes are non-breaking clarifications.

## Fallback Rules

- Missing semantic key: use host-provided default and log diagnostic.
- Unknown theme adapter id: host keeps current valid theme and logs diagnostic.
- Invalid package payload: host rejects activation.

## Error Handling Expectations

- Plugin-facing diagnostics should be actionable and stable.
- No plugin should crash host theme application pipeline.
- Host remains source of truth for active theme selection.
