#include "DarkGlassThemePlugin.h"

#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"

PluginManifest DarkGlassThemePlugin::GetManifest() const
{
    PluginManifest m;
    m.id             = L"community.dark_glass_theme";
    m.displayName    = L"Dark Glass Theme";
    m.version        = L"1.2.0";
    m.description    = L"Translucent dark theme for fence windows with a frosted-glass look.";
    m.enabledByDefault = true;
    m.capabilities   = {L"appearance", L"settings_pages"};
    return m;
}

bool DarkGlassThemePlugin::Initialize(const PluginContext& context)
{
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

    // Read persisted values and apply the theme
    const bool enabled = (context.settingsRegistry->GetValue(
        L"dark_glass.style.enabled", L"true") == L"true");

    if (enabled)
    {
        // Future: call into an appearance API to set colours, DWM attributes, etc.
        // For now the settings are declared and persisted for use by FenceWindow.
    }

    return true;
}

void DarkGlassThemePlugin::Shutdown()
{
    // Future: restore default fence window colours here.
}
