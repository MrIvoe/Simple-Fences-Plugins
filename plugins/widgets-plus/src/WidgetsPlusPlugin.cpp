#include "WidgetsPlusPlugin.h"

#include "core/CommandDispatcher.h"
#include "core/Diagnostics.h"
#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"


PluginManifest WidgetsPlusPlugin::GetManifest() const
{
    PluginManifest m;
    m.id = L"community.widgets_plus";
    m.displayName = L"Widgets Plus";
    m.version = L"1.0.2";
    m.description = L"Provides embeddable utility widgets with bounded refresh and persisted state.";
    m.minHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.capabilities = {L"widgets", L"commands", L"settings_pages", L"tray_contributions"};
    return m;
}

bool WidgetsPlusPlugin::Initialize(const PluginContext& context)
{
    m_context = context;
    if (!m_context.commandDispatcher || !m_context.settingsRegistry)
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Error(L"WidgetsPlusPlugin: Missing required host services");
        }
        return false;
    }

    RegisterSettings();
    RegisterMenus();
    RegisterCommands();
    LogInfo(L"Initialized");
    return true;
}

void WidgetsPlusPlugin::Shutdown()
{
    LogInfo(L"Shutdown");
}

void WidgetsPlusPlugin::RegisterSettings() const
{
    PluginSettingsPage page;
    page.pluginId = L"community.widgets_plus";
    page.pageId = L"widgets.general";
    page.title = L"Widgets Plus";
    page.order = 70;

    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 60, false);
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.enabled", L"Enable widgets", L"Master toggle for widgets functionality.", SettingsFieldType::Bool, L"true", {}, 10});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.refresh_seconds_default", L"Default refresh interval (s)", L"Default refresh interval for widget content.", SettingsFieldType::Int, L"60", {}, 20});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.pause_on_battery_or_fullscreen", L"Pause on battery or fullscreen", L"Pause widget refresh for power or fullscreen workloads.", SettingsFieldType::Bool, L"true", {}, 30});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.clock.mode", L"Clock mode", L"Clock widget render mode.", SettingsFieldType::Enum, L"digital", {{L"digital", L"Digital"}, {L"analog", L"Analog"}}, 40});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.clock.use_24h", L"Clock use 24-hour format", L"Use 24-hour display for digital clock.", SettingsFieldType::Bool, L"false", {}, 50});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.notes.auto_save", L"Notes auto-save", L"Automatically save note widget content.", SettingsFieldType::Bool, L"true", {}, 60});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.notes.max_chars", L"Notes max characters", L"Maximum characters for notes widget content.", SettingsFieldType::Int, L"5000", {}, 70});

    page.fields.push_back(SettingsFieldDescriptor{L"widgets.checklist.auto_sort_checked", L"Checklist — move checked to bottom", L"Automatically move checked items to the bottom of the list.", SettingsFieldType::Bool, L"true", {}, 80});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.checklist.persist_across_sessions", L"Checklist — persist state", L"Save and restore checklist check state between sessions.", SettingsFieldType::Bool, L"true", {}, 90});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.checklist.max_items", L"Checklist — max items", L"Maximum number of items displayed in a checklist widget.", SettingsFieldType::Int, L"100", {}, 100});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.checklist.show_item_count", L"Checklist — show item count", L"Display a checked/total count badge on the checklist widget.", SettingsFieldType::Bool, L"true", {}, 110});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.animations.enabled", L"Enable widget animations", L"Allow widgets to use animated transitions for state changes.", SettingsFieldType::Bool, L"true", {}, 120});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.animations.transition_ms", L"Animation transition (ms)", L"Duration of widget state-change animations in milliseconds.", SettingsFieldType::Int, L"180", {}, 130});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.cpu.show_graph", L"CPU — show usage graph", L"Show a mini usage graph in the CPU/system info widget.", SettingsFieldType::Bool, L"true", {}, 140});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.cpu.refresh_ms", L"CPU — refresh interval (ms)", L"How often the CPU widget polls system utilization data.", SettingsFieldType::Int, L"2000", {}, 150});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.weather.enabled", L"Weather — enable widget", L"Enable the weather information widget type.", SettingsFieldType::Bool, L"false", {}, 160});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.weather.location", L"Weather — location", L"City name or coordinates used for weather lookups (e.g. London or 51.5,-0.1). Leave blank to auto-detect.", SettingsFieldType::String, L"", {}, 170});
    page.fields.push_back(SettingsFieldDescriptor{L"widgets.weather.unit", L"Weather — temperature unit", L"Preferred temperature unit for weather display.", SettingsFieldType::Enum, L"celsius", {{L"celsius", L"Celsius"}, {L"fahrenheit", L"Fahrenheit"}}, 180});

    m_context.settingsRegistry->RegisterPage(std::move(page));
}

void WidgetsPlusPlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Add Clock Widget", L"widgets.add.clock", 510, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Add Notes Widget", L"widgets.add.notes", 520, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Add Checklist Widget", L"widgets.add.checklist", 530, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Refresh All Widgets", L"widgets.refresh_all", 540, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Pause Widgets", L"widgets.pause_toggle", 550, false});
}

void WidgetsPlusPlugin::RegisterCommands() const
{
    m_context.commandDispatcher->RegisterCommand(L"widgets.add.clock", [this](const CommandContext& command) { HandleAddClock(command); });
    m_context.commandDispatcher->RegisterCommand(L"widgets.add.notes", [this](const CommandContext& command) { HandleAddNotes(command); });
    m_context.commandDispatcher->RegisterCommand(L"widgets.add.checklist", [this](const CommandContext& command) { HandleAddChecklist(command); });
    m_context.commandDispatcher->RegisterCommand(L"widgets.refresh_all", [this](const CommandContext& command) { HandleRefreshAll(command); });
    m_context.commandDispatcher->RegisterCommand(L"widgets.pause_toggle", [this](const CommandContext& command) { HandlePauseToggle(command); });
}

void WidgetsPlusPlugin::HandleAddClock(const CommandContext&) const
{
    const std::wstring mode = GetString(L"widgets.clock.mode", L"digital");
    const bool use24h = GetBool(L"widgets.clock.use_24h", false);
    LogInfo(L"Add clock widget requested: mode=" + mode + L", use24h=" + (use24h ? L"true" : L"false"));
    Notify(L"Add clock widget requested.");
}

void WidgetsPlusPlugin::HandleAddNotes(const CommandContext&) const
{
    const bool autoSave = GetBool(L"widgets.notes.auto_save", true);
    const int maxChars = GetInt(L"widgets.notes.max_chars", 5000);
    LogInfo(L"Add notes widget requested: auto_save=" + std::wstring(autoSave ? L"true" : L"false") + L", max_chars=" + std::to_wstring(maxChars));
    Notify(L"Add notes widget requested.");
}

void WidgetsPlusPlugin::HandleAddChecklist(const CommandContext&) const
{
    LogInfo(L"Add checklist widget requested");
    Notify(L"Add checklist widget requested.");
}

void WidgetsPlusPlugin::HandleRefreshAll(const CommandContext&) const
{
    if (m_paused)
    {
        LogInfo(L"Refresh skipped: widgets are paused");
        return;
    }

    if (m_context.appCommands)
    {
        const auto ids = m_context.appCommands->GetAllSpaceIds();
        for (const auto& id : ids)
        {
            const SpaceMetadata meta = m_context.appCommands->GetSpaceMetadata(id);
            if (meta.contentType == L"widget_panel")
            {
                RefreshSpaceWithThrottle(id);
            }
        }
    }

    LogInfo(L"Refresh all widgets requested");
    Notify(L"Refresh all widgets requested.");
}

void WidgetsPlusPlugin::HandlePauseToggle(const CommandContext&) const
{
    m_paused = !m_paused;
    LogInfo(m_paused ? L"Widgets paused" : L"Widgets resumed");
    Notify(m_paused ? L"Widgets paused." : L"Widgets resumed.");
}

bool WidgetsPlusPlugin::GetBool(const std::wstring& key, bool fallback) const
{
    return GetString(key, fallback ? L"true" : L"false") == L"true";
}

int WidgetsPlusPlugin::GetInt(const std::wstring& key, int fallback) const
{
    const std::wstring text = GetString(key, std::to_wstring(fallback));
    try
    {
        return std::stoi(text);
    }
    catch (...)
    {
        return fallback;
    }
}

std::wstring WidgetsPlusPlugin::GetString(const std::wstring& key, const std::wstring& fallback) const
{
    return m_context.settingsRegistry ? m_context.settingsRegistry->GetValue(key, fallback) : fallback;
}

void WidgetsPlusPlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[WidgetsPlus][Notification] " + message);
}

void WidgetsPlusPlugin::RefreshSpaceWithThrottle(const std::wstring& spaceId) const
{
    if (!m_context.appCommands || spaceId.empty())
    {
        return;
    }

    int seconds = GetInt(L"plugin.refresh_interval_seconds", 60);
    if (seconds < 1)
    {
        seconds = 1;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto it = m_lastRefreshAtBySpace.find(spaceId);
    if (it != m_lastRefreshAtBySpace.end() && (now - it->second) < std::chrono::seconds(seconds))
    {
        LogInfo(L"Widget refresh throttled by plugin.refresh_interval_seconds");
        return;
    }

    m_lastRefreshAtBySpace[spaceId] = now;
    m_context.appCommands->RefreshSpace(spaceId);
}

void WidgetsPlusPlugin::LogInfo(const std::wstring& message) const
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"[WidgetsPlus] " + message);
    }
}



