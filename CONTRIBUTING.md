# Contributing to Simple-Fences-Plugins

Thank you for wanting to contribute a plugin! Follow the steps below to get your plugin reviewed and merged.

---

## Before you start

- Read [PLUGIN_GUIDE.md](PLUGIN_GUIDE.md) — it covers the full API, field types, and naming rules.
- Start from [plugin-template](plugin-template) unless you have a strong reason not to.
- Check existing plugins under `plugins/` to avoid duplicating functionality.
- Open an issue first if your plugin needs a new capability that the host doesn't yet expose — we can coordinate the API addition in the main repo.

---

## Adding a plugin

### 1  Fork and clone

```bash
git clone https://github.com/<your-username>/Simple-Fences-Plugins.git
cd Simple-Fences-Plugins
```

### 2  Create your plugin folder

```text
plugins/
  my-plugin-name/         ← kebab-case, unique
    plugin.json           ← required manifest
    README.md             ← encouraged
    src/
      MyPlugin.h
      MyPlugin.cpp
```

### 3  Fill in `plugin.json`

```json
{
  "id": "community.my_plugin_name",
  "displayName": "My Plugin Name",
  "version": "1.0.0",
  "description": "One sentence describing what the plugin does.",
  "author": "Your Name or GitHub handle",
  "minHostVersion": "0.0.009",
  "capabilities": ["settings_pages"],
  "repository": "https://github.com/<you>/<your-plugin-repo>"
}
```

### 4  Self-review checklist

- [ ] `plugin.json` is valid JSON and has all required keys
- [ ] Plugin id uses `community.` prefix and matches the folder name (snake_case)
- [ ] All settings keys follow `<plugin_short>.<page>.<field>` pattern
- [ ] Content-provider plugins use `providerId` and `contentType` correctly in code
- [ ] Plugin compiles cleanly against the latest SimpleFences `main` branch
- [ ] `SimpleFences.exe` was not already running during verification
- [ ] Settings page changes persist after restart
- [ ] Plugin does not write to arbitrary filesystem paths (only `SettingsStore` / own AppData subfolder)
- [ ] No hard-coded absolute paths or machine-specific values

### 5  Open a pull request

Title format: `Add plugin: <displayName>`

Include in the PR description:

- What the plugin does
- Which capability/capabilities it uses
- Any host API additions required (link the main repo PR if applicable)

---

## Code style

- C++17, matching the host's style (4-space indent, `PascalCase` types, `camelCase` locals)
- No third-party libraries beyond what the host already vendors (currently: `nlohmann/json`)
- Keep plugin code self-contained — do not modify host source files in your PR
- Prefer focused, realistic settings over placeholder configuration fields

---

## Review process

1. Automated: the CI check verifies `plugin.json` schema validity.
2. Manual review by a maintainer for API fit, naming, and host compatibility.
3. Once approved, your plugin appears in the hub listing in `README.md`.

---

## Questions?

Open an issue or start a discussion in the main [SimpleFences repo](https://github.com/MrIvoe/IVOESimpleFences).
