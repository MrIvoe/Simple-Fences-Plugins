#include "VisualModesPlugin.h"

#include "core/CommandDispatcher.h"
#include "core/Diagnostics.h"
#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

#include <array>
#include <vector>

namespace
{
    std::wstring NormalizeWin32ThemeId(std::wstring themeId)
    {
        // Backward compatibility: older plugin builds persisted underscore ids.
        for (auto& ch : themeId)
        {
            if (ch == L'_')
            {
                ch = L'-';
            }
        }

        if (themeId.empty())
        {
            return L"graphite-office";
        }

        return themeId;
    }

    const std::vector<SettingsEnumOption>& Win32ThemeOptions()
    {
        static const std::vector<SettingsEnumOption> options = {
            {L"amber-terminal", L"Amber Terminal"},
            {L"arctic-glass", L"Arctic Glass"},
            {L"aurora-light", L"Aurora Light"},
            {L"brass-steampunk", L"Brass Steampunk"},
            {L"copper-foundry", L"Copper Foundry"},
            {L"emerald-ledger", L"Emerald Ledger"},
            {L"forest-organic", L"Forest Organic"},
            {L"graphite-office", L"Graphite Office"},
            {L"harbor-blue", L"Harbor Blue"},
            {L"ivory-bureau", L"Ivory Bureau"},
            {L"mono-minimal", L"Mono Minimal"},
            {L"neon-cyberpunk", L"Neon Cyberpunk"},
            {L"nocturne-dark", L"Nocturne Dark"},
            {L"nova-futuristic", L"Nova Futuristic"},
            {L"olive-terminal", L"Olive Terminal"},
            {L"pop-colorburst", L"Pop Colorburst"},
            {L"rose-paper", L"Rose Paper"},
            {L"storm-steel", L"Storm Steel"},
            {L"sunset-retro", L"Sunset Retro"},
            {L"tape-lo-fi", L"Tape Lo-Fi"},
            {L"custom", L"Custom"},
        };

        return options;
    }

    bool IsLikelyDarkTheme(const std::wstring& themeKey)
    {
        static const std::array<std::wstring, 10> darkKeys = {
            L"amber-terminal",
            L"arctic-glass",
            L"graphite-office",
            L"neon-cyberpunk",
            L"nocturne-dark",
            L"olive-terminal",
            L"storm-steel",
            L"harbor-blue",
            L"forest-organic",
            L"nova-futuristic",
        };

        for (const auto& key : darkKeys)
        {
            if (themeKey == key)
            {
                return true;
            }
        }

        return false;
    }

    std::wstring ThemeDisplayNameFromKey(const std::wstring& themeKey)
    {
        const std::wstring normalized = NormalizeWin32ThemeId(themeKey);
        for (const auto& option : Win32ThemeOptions())
        {
            if (option.value == normalized)
            {
                return option.label;
            }
        }

        return L"Graphite Office";
    }
}

PluginManifest VisualModesPlugin::GetManifest() const
{
    PluginManifest m;
    m.id = L"community.visual_modes";
    m.displayName = L"Visual Modes";
    m.version = L"1.1.6";
    m.description = L"Applies global or per-space visual behavior presets.";
    m.minHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.capabilities = {L"appearance", L"commands", L"tray_contributions", L"settings_pages"};
    return m;
}

bool VisualModesPlugin::Initialize(const PluginContext& context)
{
    m_context = context;
    if (!m_context.settingsRegistry || !m_context.commandDispatcher || !m_context.appCommands)
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Error(L"VisualModesPlugin: Missing required host services");
        }
        return false;
    }

    RegisterSettings();
    RegisterMenus();
    RegisterCommands();
    LogInfo(L"Initialized");
    return true;
}

void VisualModesPlugin::Shutdown()
{
    LogInfo(L"Shutdown");
}

void VisualModesPlugin::RegisterSettings() const
{
    PluginSettingsPage page;
    page.pluginId = L"community.visual_modes";
    page.pageId = L"theme.visual_modes";
    page.title = L"Visual Modes";
    page.order = 40;

    page.fields.push_back(SettingsFieldDescriptor{L"plugin.enabled", L"Enable plugin", L"Master toggle for Visual Modes behavior.", SettingsFieldType::Bool, L"true", {}, 1});
    page.fields.push_back(SettingsFieldDescriptor{L"plugin.log_actions", L"Log actions", L"Write visual mode actions to diagnostics.", SettingsFieldType::Bool, L"true", {}, 2});
    page.fields.push_back(SettingsFieldDescriptor{L"plugin.safe_mode", L"Safe mode", L"When enabled, global preset apply is blocked to prevent broad visual changes.", SettingsFieldType::Bool, L"false", {}, 3});
    page.fields.push_back(SettingsFieldDescriptor{L"plugin.default_mode", L"Default mode", L"Default visual behavior mode profile.", SettingsFieldType::Enum, L"balanced", {{L"balanced", L"Balanced"}, {L"showcase", L"Showcase"}, {L"minimal", L"Minimal"}}, 4});
    page.fields.push_back(SettingsFieldDescriptor{L"plugin.config_source", L"Config source", L"Configuration source identifier for this plugin.", SettingsFieldType::String, L"local", {}, 5});
    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 6, 300, false);

    page.fields.push_back(SettingsFieldDescriptor{
        L"theme.preset",
        L"Preset",
        L"Choose the active theme from the Win32ThemeSystem catalog.",
        SettingsFieldType::Enum,
        L"graphite-office",
        Win32ThemeOptions(),
        10});

    page.fields.push_back(SettingsFieldDescriptor{L"theme.apply_global", L"Apply globally", L"Apply preset actions to all spaces by default.", SettingsFieldType::Bool, L"true", {}, 20});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.allow_per_space_override", L"Allow per-space override", L"Allow apply/reset commands to target one space.", SettingsFieldType::Bool, L"true", {}, 30});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.source", L"Theme source", L"Theme catalog source used by this plugin.", SettingsFieldType::Enum, L"win32_theme_system", {{L"win32_theme_system", L"Win32ThemeSystem"}}, 35});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.win32.theme_id", L"Win32 theme ID", L"Canonical Win32ThemeSystem theme family identifier (kebab-case).", SettingsFieldType::String, L"graphite-office", {}, 36});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.win32.display_name", L"Win32 theme display name", L"Normalized Win32ThemeSystem display name for host-side theme bridge integration.", SettingsFieldType::String, L"Graphite Office", {}, 37});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.win32.catalog_version", L"Win32 theme catalog version", L"Catalog contract version emitted for host bridge consumers.", SettingsFieldType::String, L"2026.04.06", {}, 38});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.colors.background", L"Background color", L"Custom preset background color (#RRGGBB). Leave blank to use host resources.", SettingsFieldType::String, L"", {}, 40});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.colors.header", L"Header color", L"Custom preset header color (#RRGGBB). Leave blank to use host resources.", SettingsFieldType::String, L"", {}, 50});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.colors.border", L"Border color", L"Custom preset border color (#RRGGBB). Leave blank to use host resources.", SettingsFieldType::String, L"", {}, 60});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.colors.text", L"Text color", L"Custom preset text color (#RRGGBB). Leave blank to use host resources.", SettingsFieldType::String, L"", {}, 70});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.effects.transparency", L"Enable transparency", L"Enable transparent visual treatment when supported.", SettingsFieldType::Bool, L"false", {}, 80});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.keep_title_bar_visible", L"Keep title bar visible", L"Prevent presets from combining rollup and transparency in a way that can fully hide the space.", SettingsFieldType::Bool, L"true", {}, 85});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.effects.opacity_percent", L"Opacity percent", L"Window opacity percent for custom and glass presets.", SettingsFieldType::Int, L"88", {}, 90});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.effects.blur", L"Enable blur", L"Enable blur when host compositor supports it.", SettingsFieldType::Bool, L"true", {}, 100});
    page.fields.push_back(SettingsFieldDescriptor{L"theme.effects.corner_radius_px", L"Corner radius (px)", L"Corner radius used by supported presets.", SettingsFieldType::Int, L"8", {}, 110});

    m_context.settingsRegistry->RegisterPage(std::move(page));

    PluginSettingsPage autoSwitchPage;
    autoSwitchPage.pluginId = L"community.visual_modes";
    autoSwitchPage.pageId   = L"theme.auto_switch";
    autoSwitchPage.title    = L"Auto Switch";
    autoSwitchPage.order    = 50;

    autoSwitchPage.fields.push_back(SettingsFieldDescriptor{
        L"theme.auto_switch.enabled",
        L"Enable time-based auto switch",
        L"Automatically switch the active preset between a day and night theme based on the configured schedule.",
        SettingsFieldType::Bool, L"false", {}, 10
    });

    autoSwitchPage.fields.push_back(SettingsFieldDescriptor{
        L"theme.auto_switch.day_preset",
        L"Day preset",
        L"Preset applied during the day period when auto switch is enabled.",
        SettingsFieldType::Enum, L"graphite-office",
        Win32ThemeOptions(),
        20
    });

    autoSwitchPage.fields.push_back(SettingsFieldDescriptor{
        L"theme.auto_switch.night_preset",
        L"Night preset",
        L"Preset applied during the night period when auto switch is enabled.",
        SettingsFieldType::Enum, L"nocturne-dark",
        Win32ThemeOptions(),
        30
    });

    autoSwitchPage.fields.push_back(SettingsFieldDescriptor{
        L"theme.auto_switch.day_start_hour",
        L"Day start (hour, 0-23)",
        L"Hour at which the day period begins and the day preset is applied.",
        SettingsFieldType::Int, L"7", {}, 40
    });

    autoSwitchPage.fields.push_back(SettingsFieldDescriptor{
        L"theme.auto_switch.night_start_hour",
        L"Night start (hour, 0-23)",
        L"Hour at which the night period begins and the night preset is applied.",
        SettingsFieldType::Int, L"20", {}, 50
    });

    autoSwitchPage.fields.push_back(SettingsFieldDescriptor{
        L"theme.auto_switch.follow_system_dark_mode",
        L"Follow system dark mode",
        L"When enabled, override the schedule and switch presets whenever Windows dark mode changes.",
        SettingsFieldType::Bool, L"false", {}, 60
    });

    m_context.settingsRegistry->RegisterPage(std::move(autoSwitchPage));
}

void VisualModesPlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Switch Visual Preset", L"theme.switch", 210, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Toggle Compact Mode", L"theme.compact_toggle", 220, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Toggle Presentation Mode", L"theme.presentation_toggle", 230, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Sync Host Theme Bridge", L"theme.host_bridge_sync", 240, false});
}

void VisualModesPlugin::RegisterCommands() const
{
    m_context.commandDispatcher->RegisterCommand(L"theme.switch", [this](const CommandContext& command) { HandleThemeSwitch(command); });
    m_context.commandDispatcher->RegisterCommand(L"theme.compact_toggle", [this](const CommandContext& command) { HandleCompactToggle(command); });
    m_context.commandDispatcher->RegisterCommand(L"theme.presentation_toggle", [this](const CommandContext& command) { HandlePresentationToggle(command); });
    m_context.commandDispatcher->RegisterCommand(L"theme.apply_current_to_space", [this](const CommandContext& command) { HandleApplyCurrentToSpace(command); });
    m_context.commandDispatcher->RegisterCommand(L"theme.reset_space", [this](const CommandContext& command) { HandleResetSpace(command); });
    m_context.commandDispatcher->RegisterCommand(L"theme.host_bridge_sync", [this](const CommandContext&) { SetPreset(GetSetting(L"theme.preset", L"graphite-office")); Notify(L"Host theme bridge synchronized."); });

    // Compatibility aliases used by older/context-action routes.
    m_context.commandDispatcher->RegisterCommand(L"appearance.mode.focus", [this](const CommandContext& command) { HandleApplyCurrentToSpace(command); });
    m_context.commandDispatcher->RegisterCommand(L"appearance.mode.cycle", [this](const CommandContext& command) { HandleThemeSwitch(command); });
    m_context.commandDispatcher->RegisterCommand(L"appearance.mode.reset", [this](const CommandContext& command) { HandleResetSpace(command); });
}

void VisualModesPlugin::HandleThemeSwitch(const CommandContext&) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    std::vector<std::wstring> presets;
    for (const auto& option : Win32ThemeOptions())
    {
        presets.push_back(option.value);
    }

    const std::wstring current = NormalizeWin32ThemeId(GetSetting(L"theme.preset", L"graphite-office"));
    size_t index = 0;
    while (index < presets.size() && presets[index] != current)
    {
        ++index;
    }

    const std::wstring nextPreset = presets[(index + 1) % presets.size()];
    SetPreset(nextPreset);

    if (GetSettingBool(L"theme.apply_global", true) && !IsSafeModeEnabled())
    {
        const SpaceMetadata active = m_context.appCommands->GetActiveSpaceMetadata();
        if (!active.id.empty())
        {
            ApplyPresetToSpace(active.id, nextPreset, true);
        }
    }
}

void VisualModesPlugin::HandleCompactToggle(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const std::wstring current = NormalizeWin32ThemeId(GetSetting(L"theme.preset", L"graphite-office"));
    const std::wstring next = (current == L"mono-minimal") ? L"graphite-office" : L"mono-minimal";
    SetPreset(next);

    const std::wstring spaceId = ResolveTargetSpaceId(command);
    if (!spaceId.empty())
    {
        const bool applyToAll = GetSettingBool(L"theme.apply_global", true);
        ApplyPresetToSpace(spaceId, next, applyToAll);
    }
}

void VisualModesPlugin::HandlePresentationToggle(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const std::wstring current = NormalizeWin32ThemeId(GetSetting(L"theme.preset", L"graphite-office"));
    const std::wstring next = (current == L"storm-steel") ? L"aurora-light" : L"storm-steel";
    SetPreset(next);

    const std::wstring spaceId = ResolveTargetSpaceId(command);
    if (!spaceId.empty())
    {
        const bool applyToAll = GetSettingBool(L"theme.apply_global", true);
        ApplyPresetToSpace(spaceId, next, applyToAll);
    }
}

void VisualModesPlugin::HandleApplyCurrentToSpace(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    if (!GetSettingBool(L"theme.allow_per_space_override", true))
    {
        return;
    }

    const std::wstring spaceId = ResolveTargetSpaceId(command);
    if (spaceId.empty())
    {
        return;
    }

    ApplyPresetToSpace(spaceId, NormalizeWin32ThemeId(GetSetting(L"theme.preset", L"graphite-office")), false);
}

void VisualModesPlugin::HandleResetSpace(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    if (!GetSettingBool(L"theme.allow_per_space_override", true))
    {
        return;
    }

    const std::wstring spaceId = ResolveTargetSpaceId(command);
    if (spaceId.empty())
    {
        return;
    }

    ApplyPresetToSpace(spaceId, L"graphite-office", false);
}

std::wstring VisualModesPlugin::GetSetting(const std::wstring& key, const std::wstring& fallback) const
{
    return m_context.settingsRegistry ? m_context.settingsRegistry->GetValue(key, fallback) : fallback;
}

bool VisualModesPlugin::GetSettingBool(const std::wstring& key, bool fallback) const
{
    return GetSetting(key, fallback ? L"true" : L"false") == L"true";
}

bool VisualModesPlugin::IsPluginEnabled() const
{
    return GetSettingBool(L"plugin.enabled", true);
}

bool VisualModesPlugin::ShouldLogActions() const
{
    return GetSettingBool(L"plugin.log_actions", true);
}

bool VisualModesPlugin::IsSafeModeEnabled() const
{
    return GetSettingBool(L"plugin.safe_mode", true);
}

int VisualModesPlugin::GetRefreshIntervalSeconds() const
{
    int seconds = 300;
    try
    {
        seconds = std::stoi(GetSetting(L"plugin.refresh_interval_seconds", L"300"));
    }
    catch (...)
    {
        seconds = 300;
    }

    if (seconds < 1)
    {
        seconds = 1;
    }

    return seconds;
}

void VisualModesPlugin::Notify(const std::wstring& message) const
{
    if (!GetSettingBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[VisualModes][Notification] " + message);
}

bool VisualModesPlugin::CanApplyNow() const
{
    const auto now = std::chrono::steady_clock::now();
    const auto interval = std::chrono::seconds(GetRefreshIntervalSeconds());
    if (m_lastApplyAt.time_since_epoch().count() != 0 && (now - m_lastApplyAt) < interval)
    {
        LogInfo(L"Apply throttled by plugin.refresh_interval_seconds");
        return false;
    }

    m_lastApplyAt = now;
    return true;
}

void VisualModesPlugin::SetPreset(const std::wstring& preset) const
{
    if (!m_context.settingsRegistry)
    {
        return;
    }

    const std::wstring normalized = NormalizeWin32ThemeId(preset);
    m_context.settingsRegistry->SetValue(L"theme.preset", normalized);
    m_context.settingsRegistry->SetValue(L"theme.win32.display_name", ThemeDisplayNameFromKey(normalized));
    m_context.settingsRegistry->SetValue(L"theme.win32.theme_id", normalized);
    m_context.settingsRegistry->SetValue(L"theme.source", L"win32_theme_system");
    LogInfo(L"Preset switched to " + normalized);
}

std::wstring VisualModesPlugin::ResolveTargetSpaceId(const CommandContext& command) const
{
    if (!command.space.id.empty())
    {
        return command.space.id;
    }

    const CommandContext activeCommand = m_context.appCommands->GetCurrentCommandContext();
    if (!activeCommand.space.id.empty())
    {
        return activeCommand.space.id;
    }

    return m_context.appCommands->GetActiveSpaceMetadata().id;
}

SpacePresentationSettings VisualModesPlugin::BuildPresentationFromPreset(const std::wstring& preset) const
{
    SpacePresentationSettings settings;

    if (preset == L"custom")
    {
        settings.textOnlyMode = false;
        settings.rollupWhenNotHovered = false;
        settings.transparentWhenNotHovered = GetSettingBool(L"theme.effects.transparency", false);
        settings.labelsOnHover = true;
        settings.iconSpacingPreset = L"comfortable";
    }
    else if (IsLikelyDarkTheme(preset))
    {
        settings.textOnlyMode = false;
        settings.rollupWhenNotHovered = false;
        settings.transparentWhenNotHovered = false;
        settings.labelsOnHover = true;
        settings.iconSpacingPreset = L"comfortable";
    }
    else
    {
        settings.textOnlyMode = false;
        settings.rollupWhenNotHovered = false;
        settings.transparentWhenNotHovered = false;
        settings.labelsOnHover = true;
        settings.iconSpacingPreset = L"spacious";
    }

    if (GetSettingBool(L"theme.keep_title_bar_visible", true) && settings.rollupWhenNotHovered)
    {
        settings.transparentWhenNotHovered = false;
    }

    return settings;
}

void VisualModesPlugin::ApplyPresetToSpace(const std::wstring& spaceId, const std::wstring& preset, bool applyToAll) const
{
    if (!m_context.appCommands || spaceId.empty())
    {
        return;
    }

    if (!CanApplyNow())
    {
        return;
    }

    SpacePresentationSettings settings = BuildPresentationFromPreset(preset);
    settings.applyToAll = applyToAll && !IsSafeModeEnabled();
    m_context.appCommands->UpdateSpacePresentation(spaceId, settings);
    Notify(L"Visual preset applied.");
}

void VisualModesPlugin::LogInfo(const std::wstring& message) const
{
    if (m_context.diagnostics && ShouldLogActions())
    {
        m_context.diagnostics->Info(L"[VisualModes] " + message);
    }
}



