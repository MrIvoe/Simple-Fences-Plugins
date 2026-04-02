# PowerShell Workspace Fence

Prototype plugin for a PowerShell-driven fence experience in SimpleFences.

## Current phase

This plugin is intentionally a prototype. It registers a PowerShell-oriented fence content type and defines the persisted settings contract that a richer host integration can consume later.

## What it does now

- registers the `powershell_workspace` content type
- declares startup, view, and safety settings
- gives the host a stable provider id and content type for future persistence and UI wiring

## What the host still needs

- a child-window or terminal-host surface inside a fence
- process lifecycle callbacks for starting and reusing PowerShell sessions
- provider hooks that can materialize workspace content instead of only metadata

## Suggested host direction

The most practical design is a fence that can open into one of these modes:

1. embedded terminal
2. script dashboard for a startup script or task profile
3. pinned workspace showing recent scripts, modules, and output logs

This sample keeps the configuration side ready while the host surface catches up.