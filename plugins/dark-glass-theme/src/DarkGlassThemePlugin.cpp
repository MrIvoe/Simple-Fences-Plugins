#include "DarkGlassThemePlugin.h"

#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

PluginManifest DarkGlassThemePlugin::GetManifest() const
{
    PluginManifest m;
    m.id             = L"community.dark_glass_theme";
    m.displayName    = L"Dark Glass Theme";
    m.version = L"1.2.3";
    m.description    = L"Translucent dark theme for fence windows with a frosted-glass look.";
    m.enabledByDefault = true;
    m.capabilities   = {L"appearance", L"settings_pages", L"commands", L"tray_contributions"};
    return m;
}

bool DarkGlassThemePlugin::Initialize(const PluginContext& context)
{
    m_context = context;

    if (!context.settingsRegistry)
    {
        return true; // no registry – run with defaults
    }

    // --- Settings page ---
    PluginSettingsPage page;
    page.pluginId = L"community.dark_glass_theme";
    page.pageId   = L"dark_glass.style";
    page.title    = L"Glass Style";
    page.order    = 10;

    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 60, false);

    page.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.style.enabled",
        L"Enable dark glass theme",
        L"Apply the translucent dark style to all fence windows.",
        SettingsFieldType::Bool, L"true", {}, 10
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.style.opacity",
        L"Background opacity (0-100)",
        L"Percentage opacity of the fence background. 100 = fully opaque.",
        SettingsFieldType::Int, L"80", {}, 20
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.style.blur_mode",
        L"Blur mode",
        L"Amount of background blur applied to the glass surface.",
        SettingsFieldType::Enum, L"medium",
        {
            {L"none",   L"None"},
            {L"light",  L"Light blur"},
            {L"medium", L"Medium blur"},
            {L"heavy",  L"Heavy blur"},
        },
        30
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.style.corner_radius",
        L"Corner radius (px)",
        L"Rounded corner radius for fence windows (0 = square corners).",
        SettingsFieldType::Int, L"8", {}, 40
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.style.border_intensity",
        L"Border intensity (0-100)",
        L"Strength of the glass edge highlight around each fence.",
        SettingsFieldType::Int, L"35", {}, 50
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.style.tint_hex",
        L"Tint color",
        L"Optional hex tint such as #1C2430. Leave blank to use the default dark tint.",
        SettingsFieldType::String, L"#1C2430", {}, 60
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.style.show_border",
        L"Show glass border",
        L"Draw a subtle border around the fence surface.",
        SettingsFieldType::Bool, L"true", {}, 70
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.style.header_opacity",
        L"Header opacity (0-100)",
        L"Opacity for the fence title or header region when the theme is applied.",
        SettingsFieldType::Int, L"92", {}, 80
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.style.surface_variant",
        L"Surface variant",
        L"Choose the overall glass treatment used for the fence body.",
        SettingsFieldType::Enum, L"acrylic",
        {
            {L"flat", L"Flat tint"},
            {L"mist", L"Mist glass"},
            {L"acrylic", L"Acrylic glass"},
        },
        90
    });

    context.settingsRegistry->RegisterPage(std::move(page));

    PluginSettingsPage behaviorPage;
    behaviorPage.pluginId = L"community.dark_glass_theme";
    behaviorPage.pageId   = L"dark_glass.behavior";
    behaviorPage.title    = L"Glass Behavior";
    behaviorPage.order    = 20;

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.behavior.shadow_strength",
        L"Shadow strength",
        L"Depth of the fence drop shadow.",
        SettingsFieldType::Enum, L"medium",
        {
            {L"off", L"Off"},
            {L"light", L"Light"},
            {L"medium", L"Medium"},
            {L"strong", L"Strong"},
        },
        10
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.behavior.compact_header",
        L"Compact title area",
        L"Use a tighter title/header spacing for smaller fences.",
        SettingsFieldType::Bool, L"false", {}, 20
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.behavior.noise_overlay",
        L"Film grain overlay",
        L"Add a subtle noise texture so the glass does not look flat.",
        SettingsFieldType::Bool, L"true", {}, 30
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.behavior.active_contrast",
        L"Active fence contrast",
        L"Boost contrast when a fence is focused or hovered.",
        SettingsFieldType::Enum, L"medium",
        {
            {L"off", L"Off"},
            {L"low", L"Low"},
            {L"medium", L"Medium"},
            {L"high", L"High"},
        },
        40
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.behavior.inactive_blur",
        L"Inactive fence blur",
        L"Reduce or preserve blur intensity for fences that are not focused.",
        SettingsFieldType::Enum, L"reduced",
        {
            {L"off", L"Off"},
            {L"reduced", L"Reduced"},
            {L"same_as_active", L"Same as active"},
        },
        50
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.behavior.reduce_transparency_on_battery",
        L"Reduce transparency on battery",
        L"Use a less expensive visual mode when the system is on battery power.",
        SettingsFieldType::Bool, L"true", {}, 60
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.behavior.animate_focus_transition",
        L"Animate focus transitions",
        L"Fade between inactive and active visual states rather than switching instantly.",
        SettingsFieldType::Bool, L"true", {}, 70
    });

    context.settingsRegistry->RegisterPage(std::move(behaviorPage));

    PluginSettingsPage palettePage;
    palettePage.pluginId = L"community.dark_glass_theme";
    palettePage.pageId   = L"dark_glass.palette";
    palettePage.title    = L"Palette";
    palettePage.order    = 30;

    palettePage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.palette.accent_hex",
        L"Accent color",
        L"Highlight color for focused fence borders and interactive elements (#RRGGBB). Leave blank to use the system accent.",
        SettingsFieldType::String, L"", {}, 10
    });

    palettePage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.palette.icon_tint_mode",
        L"Icon tint mode",
        L"Controls how the theme applies tinting to fence item icons.",
        SettingsFieldType::Enum, L"subtle",
        {
            {L"none",   L"None"},
            {L"subtle", L"Subtle tint"},
            {L"match",  L"Match tint color"},
        },
        20
    });

    palettePage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.palette.icon_tint_hex",
        L"Icon tint color",
        L"Color applied to icons when tint mode is not None (#RRGGBB). Leave blank for the automatic accent color.",
        SettingsFieldType::String, L"", {}, 30
    });

    palettePage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.palette.high_contrast_mode",
        L"High contrast mode",
        L"Increase text and border contrast for accessibility. Overrides blur and tint settings.",
        SettingsFieldType::Bool, L"false", {}, 40
    });

    palettePage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.palette.text_scale_percent",
        L"Text scale (%)",
        L"Scale factor applied to fence label text when the theme is active. 100 = default size.",
        SettingsFieldType::Int, L"100", {}, 50
    });

    palettePage.fields.push_back(SettingsFieldDescriptor{
        L"dark_glass.palette.separator_color_hex",
        L"Separator color",
        L"Color of section dividers inside fence panels (#RRGGBB). Leave blank to derive from the tint color.",
        SettingsFieldType::String, L"", {}, 60
    });

    context.settingsRegistry->RegisterPage(std::move(palettePage));

    RegisterMenus();
    RegisterCommands();

    // Read persisted values and apply the theme
    const bool enabled = (context.settingsRegistry->GetValue(
        L"dark_glass.style.enabled", L"true") == L"true");

    if (enabled)
    {
        // Future: call into an appearance API to set colours, DWM attributes, etc.
        // For now the settings are declared and persisted for use by FenceWindow.
        RefreshAllFencesWithThrottle();
        Notify(L"Dark Glass theme applied.");
    }

    return true;
}

void DarkGlassThemePlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{
        MenuSurface::Tray,
        L"Reapply Dark Glass Theme",
        L"dark_glass.reapply_theme",
        245,
        false});

    m_context.menuRegistry->Register(MenuContribution{
        MenuSurface::Tray,
        L"Toggle Dark Glass Preview",
        L"dark_glass.preview_toggle",
        246,
        false});
}

void DarkGlassThemePlugin::RegisterCommands() const
{
    if (!m_context.commandDispatcher)
    {
        return;
    }

    m_context.commandDispatcher->RegisterCommand(
        L"dark_glass.reapply_theme",
        [this](const CommandContext& command)
        {
            HandleReapplyTheme(command);
        });

    m_context.commandDispatcher->RegisterCommand(
        L"dark_glass.preview_toggle",
        [this](const CommandContext& command)
        {
            HandlePreviewToggle(command);
        });
}

void DarkGlassThemePlugin::HandleReapplyTheme(const CommandContext&) const
{
    if (!GetBool(L"dark_glass.style.enabled", true))
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Info(L"[DarkGlassTheme] Reapply skipped because dark_glass.style.enabled is false");
        }
        Notify(L"Dark Glass theme is disabled.");
        return;
    }

    ApplyThemeToAllFences(true);
    Notify(L"Dark Glass theme reapplied.");
}

void DarkGlassThemePlugin::HandlePreviewToggle(const CommandContext&) const
{
    if (!m_context.settingsRegistry)
    {
        return;
    }

    const bool currentlyEnabled = GetBool(L"dark_glass.style.enabled", true);
    const bool nextEnabled = !currentlyEnabled;
    m_context.settingsRegistry->SetValue(L"dark_glass.style.enabled", nextEnabled ? L"true" : L"false");

    ApplyThemeToAllFences(true);

    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(
            nextEnabled
                ? L"[DarkGlassTheme] Preview toggled on"
                : L"[DarkGlassTheme] Preview toggled off");
    }

    Notify(nextEnabled ? L"Dark Glass preview enabled." : L"Dark Glass preview disabled.");
}

void DarkGlassThemePlugin::Shutdown()
{
    // Future: restore default fence window colours here.
    Notify(L"Dark Glass theme shutdown.");
}

bool DarkGlassThemePlugin::GetBool(const std::wstring& key, bool fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    return m_context.settingsRegistry->GetValue(key, fallback ? L"true" : L"false") == L"true";
}

int DarkGlassThemePlugin::GetInt(const std::wstring& key, int fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    try
    {
        return std::stoi(m_context.settingsRegistry->GetValue(key, std::to_wstring(fallback)));
    }
    catch (...)
    {
        return fallback;
    }
}

void DarkGlassThemePlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[DarkGlassTheme][Notification] " + message);
}

void DarkGlassThemePlugin::RefreshAllFencesWithThrottle() const
{
    ApplyThemeToAllFences(false);
}

void DarkGlassThemePlugin::ApplyThemeToAllFences(bool bypassThrottle) const
{
    if (!m_context.appCommands)
    {
        return;
    }

    int seconds = GetInt(L"plugin.refresh_interval_seconds", 60);
    if (seconds < 1)
    {
        seconds = 1;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!bypassThrottle && m_lastApplyAt.time_since_epoch().count() != 0 && (now - m_lastApplyAt) < std::chrono::seconds(seconds))
    {
        return;
    }

    m_lastApplyAt = now;
    const auto ids = m_context.appCommands->GetAllFenceIds();
    for (const auto& id : ids)
    {
        m_context.appCommands->RefreshFence(id);
    }
}


