#include "ClockWidgetPlugin.h"

#include "core/Diagnostics.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"


PluginManifest ClockWidgetPlugin::GetManifest() const
{
    PluginManifest m;
    m.id           = L"community.clock_widget";
    m.displayName  = L"Clock Widget";
    m.version = L"1.2.2";
    m.description  = L"Configurable digital, analogue, and dashboard clock widget for a space panel.";
    m.minHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.capabilities = {L"widgets", L"settings_pages"};
    return m;
}

bool ClockWidgetPlugin::Initialize(const PluginContext& context)
{
    m_context = context;

    if (!context.settingsRegistry)
    {
        return true;
    }

    // --- Clock appearance page ---
    PluginSettingsPage page;
    page.pluginId = L"community.clock_widget";
    page.pageId   = L"clock.display";
    page.title    = L"Clock Display";
    page.order    = 10;

    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 60, false);

    page.fields.push_back(SettingsFieldDescriptor{
        L"clock.display.style",
        L"Clock style",
        L"Choose the visual presentation used inside the space widget.",
        SettingsFieldType::Enum, L"digital",
        {
            {L"digital",  L"Digital"},
            {L"analogue", L"Analogue"},
            {L"dashboard", L"Dashboard"},
        },
        10
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"clock.display.show_seconds",
        L"Show seconds",
        L"Include the seconds hand / digit in the clock display.",
        SettingsFieldType::Bool, L"true", {}, 20
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"clock.display.use_24h",
        L"24-hour format",
        L"Use 24-hour (military) time format instead of 12-hour AM/PM.",
        SettingsFieldType::Bool, L"false", {}, 30
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"clock.display.timezone",
        L"Timezone override",
        L"IANA timezone name (e.g. America/New_York). Leave blank to use the system timezone.",
        SettingsFieldType::String, L"", {}, 40
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"clock.display.show_date",
        L"Show date",
        L"Display the current date under or beside the clock face.",
        SettingsFieldType::Bool, L"true", {}, 50
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"clock.display.date_style",
        L"Date style",
        L"Choose how the date is rendered when date display is enabled.",
        SettingsFieldType::Enum, L"weekday_short",
        {
            {L"compact", L"Compact"},
            {L"weekday_short", L"Weekday + short date"},
            {L"weekday_full", L"Full weekday + long date"},
        },
        60
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"clock.display.alignment",
        L"Content alignment",
        L"Align the clock contents within the widget area.",
        SettingsFieldType::Enum, L"center",
        {
            {L"left", L"Left"},
            {L"center", L"Center"},
            {L"right", L"Right"},
        },
        70
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"clock.display.scale_percent",
        L"Display scale (%)",
        L"Scale the clock face or text size relative to the widget bounds.",
        SettingsFieldType::Int, L"100", {}, 80
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"clock.display.show_day_period",
        L"Show AM/PM marker",
        L"Show an AM/PM marker when 12-hour format is active.",
        SettingsFieldType::Bool, L"true", {}, 90
    });

    context.settingsRegistry->RegisterPage(std::move(page));

    PluginSettingsPage behaviorPage;
    behaviorPage.pluginId = L"community.clock_widget";
    behaviorPage.pageId   = L"clock.behavior";
    behaviorPage.title    = L"Clock Behavior";
    behaviorPage.order    = 20;

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"clock.behavior.refresh_ms",
        L"Refresh interval (ms)",
        L"How often the widget refreshes itself. Lower values feel smoother but repaint more often.",
        SettingsFieldType::Int, L"1000", {}, 10
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"clock.behavior.blink_separator",
        L"Blink separator",
        L"Blink the digital time separator once per second.",
        SettingsFieldType::Bool, L"true", {}, 20
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"clock.behavior.click_action",
        L"Click action",
        L"Choose what happens when the widget is clicked.",
        SettingsFieldType::Enum, L"none",
        {
            {L"none", L"Do nothing"},
            {L"open_calendar", L"Open calendar flyout"},
            {L"open_time_settings", L"Open Windows time settings"},
        },
        30
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"clock.behavior.pin_timezone_label",
        L"Show timezone label",
        L"Show the active timezone name when a custom timezone is configured.",
        SettingsFieldType::Bool, L"false", {}, 40
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"clock.behavior.pause_when_hidden",
        L"Pause when hidden",
        L"Reduce widget refresh activity while the space is minimized or occluded.",
        SettingsFieldType::Bool, L"true", {}, 50
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"clock.behavior.sync_to_system_second",
        L"Sync to system second",
        L"Align updates to whole-second boundaries to reduce visible drift.",
        SettingsFieldType::Bool, L"true", {}, 60
    });

    behaviorPage.fields.push_back(SettingsFieldDescriptor{
        L"clock.behavior.smooth_analogue_seconds",
        L"Smooth analogue seconds hand",
        L"Use smooth second-hand motion in analogue mode instead of ticking once per second.",
        SettingsFieldType::Bool, L"false", {}, 70
    });

    context.settingsRegistry->RegisterPage(std::move(behaviorPage));

    PluginSettingsPage appearancePage;
    appearancePage.pluginId = L"community.clock_widget";
    appearancePage.pageId   = L"clock.appearance";
    appearancePage.title    = L"Appearance";
    appearancePage.order    = 30;

    appearancePage.fields.push_back(SettingsFieldDescriptor{
        L"clock.appearance.font_family",
        L"Font family",
        L"Font family hint passed to the host renderer for digital clock text. Leave blank for system default.",
        SettingsFieldType::String, L"", {}, 10
    });

    appearancePage.fields.push_back(SettingsFieldDescriptor{
        L"clock.appearance.text_color_hex",
        L"Text color",
        L"Override clock text color using a hex value in #RRGGBB format. Leave blank to use host theme tokens.",
        SettingsFieldType::String, L"", {}, 20
    });

    appearancePage.fields.push_back(SettingsFieldDescriptor{
        L"clock.appearance.background_color_hex",
        L"Background color",
        L"Optional background color for the widget area (#RRGGBB). Leave blank for a transparent background.",
        SettingsFieldType::String, L"", {}, 30
    });

    appearancePage.fields.push_back(SettingsFieldDescriptor{
        L"clock.appearance.padding_px",
        L"Content padding (px)",
        L"Inner padding between the widget edge and the clock face.",
        SettingsFieldType::Int, L"8", {}, 40
    });

    appearancePage.fields.push_back(SettingsFieldDescriptor{
        L"clock.appearance.hour_hand_color_hex",
        L"Analogue hour hand color",
        L"Color of the hour hand in analogue mode (#RRGGBB). Leave blank for the theme default.",
        SettingsFieldType::String, L"", {}, 50
    });

    appearancePage.fields.push_back(SettingsFieldDescriptor{
        L"clock.appearance.second_hand_color_hex",
        L"Analogue second hand color",
        L"Accent color for the seconds hand in analogue mode (#RRGGBB). Leave blank for the theme default.",
        SettingsFieldType::String, L"", {}, 60
    });

    appearancePage.fields.push_back(SettingsFieldDescriptor{
        L"clock.appearance.hide_when_space_small",
        L"Hide when space is small",
        L"Automatically hide the clock face when the space width or height falls below the minimum.",
        SettingsFieldType::Bool, L"false", {}, 70
    });

    appearancePage.fields.push_back(SettingsFieldDescriptor{
        L"clock.appearance.min_visible_width_px",
        L"Minimum visible width (px)",
        L"Below this space width the clock is hidden when auto-hide is enabled.",
        SettingsFieldType::Int, L"120", {}, 80
    });

    context.settingsRegistry->RegisterPage(std::move(appearancePage));

    RefreshWidgetPanelsWithThrottle();
    Notify(L"Clock widget initialized.");

    return true;
}

void ClockWidgetPlugin::Shutdown()
{
    Notify(L"Clock widget shutdown.");
}

bool ClockWidgetPlugin::GetBool(const std::wstring& key, bool fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    return m_context.settingsRegistry->GetValue(key, fallback ? L"true" : L"false") == L"true";
}

int ClockWidgetPlugin::GetInt(const std::wstring& key, int fallback) const
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

void ClockWidgetPlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[ClockWidget][Notification] " + message);
}

void ClockWidgetPlugin::RefreshWidgetPanelsWithThrottle() const
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
    if (m_lastRefreshAt.time_since_epoch().count() != 0 && (now - m_lastRefreshAt) < std::chrono::seconds(seconds))
    {
        return;
    }

    m_lastRefreshAt = now;
    const auto ids = m_context.appCommands->GetAllSpaceIds();
    for (const auto& id : ids)
    {
        const SpaceMetadata space = m_context.appCommands->GetSpaceMetadata(id);
        if (space.contentType == L"widget_panel")
        {
            m_context.appCommands->RefreshSpace(id);
        }
    }
}



