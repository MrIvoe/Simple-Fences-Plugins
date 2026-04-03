#include "ClockWidgetPlugin.h"

#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

PluginManifest ClockWidgetPlugin::GetManifest() const
{
    PluginManifest m;
    m.id           = L"community.clock_widget";
    m.displayName  = L"Clock Widget";
    m.version = L"1.2.1";
    m.description  = L"Configurable digital, analogue, and dashboard clock widget for a fence panel.";
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
        L"Choose the visual presentation used inside the fence widget.",
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
        L"Reduce widget refresh activity while the fence is minimized or occluded.",
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
    const auto ids = m_context.appCommands->GetAllFenceIds();
    for (const auto& id : ids)
    {
        const FenceMetadata fence = m_context.appCommands->GetFenceMetadata(id);
        if (fence.contentType == L"widget_panel")
        {
            m_context.appCommands->RefreshFence(id);
        }
    }
}


