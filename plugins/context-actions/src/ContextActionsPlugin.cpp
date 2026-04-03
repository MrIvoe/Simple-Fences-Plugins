#include "ContextActionsPlugin.h"

#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

#include <sstream>

PluginManifest ContextActionsPlugin::GetManifest() const
{
    PluginManifest m;
    m.id = L"community.context_actions";
    m.displayName = L"Context Actions";
    m.version = L"1.2.1";
    m.description = L"Context-aware right-click actions for desktop, fence, and item workflows.";
    m.minHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.capabilities = {L"commands", L"menu_contributions", L"desktop_context", L"settings_pages"};
    return m;
}

bool ContextActionsPlugin::Initialize(const PluginContext& context)
{
    m_context = context;

    RegisterSettings();
    RegisterMenus();
    RegisterCommands();

    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"ContextActionsPlugin initialized");
    }

    return true;
}

void ContextActionsPlugin::Shutdown()
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"ContextActionsPlugin shutdown");
    }
}

void ContextActionsPlugin::RegisterSettings() const
{
    if (!m_context.settingsRegistry)
    {
        return;
    }

    PluginSettingsPage page;
    page.pluginId = L"community.context_actions";
    page.pageId = L"context.behavior";
    page.title = L"Context Actions";
    page.order = 20;

    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 60, false);
    page.fields.push_back(SettingsFieldDescriptor{L"context.enabled", L"Enable context actions", L"Master toggle for plugin-contributed context actions.", SettingsFieldType::Bool, L"true", {}, 10});
    page.fields.push_back(SettingsFieldDescriptor{L"context.show_advanced", L"Show advanced commands", L"Expose advanced context actions.", SettingsFieldType::Bool, L"false", {}, 20});
    page.fields.push_back(SettingsFieldDescriptor{L"context.compact_mode", L"Compact mode", L"Prefer compact layout for plugin context actions.", SettingsFieldType::Bool, L"true", {}, 30});
    page.fields.push_back(SettingsFieldDescriptor{L"context.group_under_submenu", L"Group under submenu", L"Group actions under a plugin submenu where supported.", SettingsFieldType::Bool, L"true", {}, 40});
    page.fields.push_back(SettingsFieldDescriptor{L"context.show_desktop_background", L"Show on desktop background", L"Show context actions on desktop context menus.", SettingsFieldType::Bool, L"true", {}, 50});
    page.fields.push_back(SettingsFieldDescriptor{L"context.show_fence_background", L"Show on fence background", L"Show context actions on fence background menus.", SettingsFieldType::Bool, L"true", {}, 60});
    page.fields.push_back(SettingsFieldDescriptor{L"context.show_fence_items", L"Show on fence items", L"Show context actions on fence item menus.", SettingsFieldType::Bool, L"true", {}, 70});

    page.fields.push_back(SettingsFieldDescriptor{L"context.max_visible_actions", L"Max visible actions", L"Maximum number of plugin actions shown directly in the menu before overflowing to a submenu.", SettingsFieldType::Int, L"8", {}, 80});
    page.fields.push_back(SettingsFieldDescriptor{L"context.show_separator_lines", L"Show separator lines", L"Show divider lines between groups of context actions.", SettingsFieldType::Bool, L"true", {}, 90});
    page.fields.push_back(SettingsFieldDescriptor{L"context.icon_style", L"Action icon style", L"Choose whether icons are shown alongside context action labels.", SettingsFieldType::Enum, L"subtle", {{L"none", L"None"}, {L"subtle", L"Subtle"}, {L"full", L"Full"}}, 100});
    page.fields.push_back(SettingsFieldDescriptor{L"context.confirm_destructive", L"Confirm destructive actions", L"Show a confirmation prompt before executing actions flagged as destructive.", SettingsFieldType::Bool, L"true", {}, 110});
    page.fields.push_back(SettingsFieldDescriptor{L"context.action_cooldown_ms", L"Action cooldown (ms)", L"Minimum milliseconds between repeated command dispatches from the same context action.", SettingsFieldType::Int, L"500", {}, 120});
    page.fields.push_back(SettingsFieldDescriptor{L"context.keyboard_shortcut_hints", L"Show keyboard shortcut hints", L"Display keyboard shortcut hints alongside context action labels where available.", SettingsFieldType::Bool, L"false", {}, 130});

    m_context.settingsRegistry->RegisterPage(std::move(page));
}

void ContextActionsPlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::DesktopContext, L"New Folder Portal Here", L"context.new_folder_portal_here", 400, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::DesktopContext, L"Sort This Fence", L"context.sort_this_fence", 410, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::FenceContext, L"Clean Up This Fence", L"context.cleanup_this_fence", 420, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::FenceContext, L"Apply Focus Visual Mode", L"context.apply_theme_this_fence", 430, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::ItemContext, L"Pin Selected", L"context.pin_selected", 435, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::ItemContext, L"Refresh Provider", L"context.refresh_provider", 440, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::ItemContext, L"Copy Item Metadata", L"context.copy_item_metadata", 450, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::DesktopContext, L"Open Plugin Settings", L"context.open_plugin_settings", 460, false});
}

void ContextActionsPlugin::RegisterCommands() const
{
    if (!m_context.commandDispatcher)
    {
        return;
    }

    m_context.commandDispatcher->RegisterCommand(L"context.new_folder_portal_here", [this](const CommandContext& command) { HandleNewFolderPortalHere(command); });
    m_context.commandDispatcher->RegisterCommand(L"context.sort_this_fence", [this](const CommandContext& command) { HandleSortThisFence(command); });
    m_context.commandDispatcher->RegisterCommand(L"context.cleanup_this_fence", [this](const CommandContext& command) { HandleCleanupThisFence(command); });
    m_context.commandDispatcher->RegisterCommand(L"context.apply_theme_this_fence", [this](const CommandContext& command) { HandleApplyThemeThisFence(command); });
    m_context.commandDispatcher->RegisterCommand(L"context.pin_selected", [this](const CommandContext& command) { HandlePinSelected(command); });
    m_context.commandDispatcher->RegisterCommand(L"context.refresh_provider", [this](const CommandContext& command) { HandleRefreshProvider(command); });
    m_context.commandDispatcher->RegisterCommand(L"context.copy_item_metadata", [this](const CommandContext& command) { HandleCopyItemMetadata(command); });
    m_context.commandDispatcher->RegisterCommand(L"context.open_plugin_settings", [this](const CommandContext& command) { HandleOpenPluginSettings(command); });
}

void ContextActionsPlugin::HandleNewFolderPortalHere(const CommandContext& command) const
{
    if (!GetBool(L"context.enabled", true))
    {
        return;
    }

    if (!m_context.commandDispatcher)
    {
        return;
    }

    CommandContext forward = command;
    forward.commandId = L"portal.new";
    if (!m_context.commandDispatcher->Dispatch(L"portal.new", forward))
    {
        LogInfo(L"new_folder_portal_here could not dispatch portal.new");
        return;
    }

    Notify(L"New Folder Portal requested.");
}

void ContextActionsPlugin::HandleSortThisFence(const CommandContext& command) const
{
    if (!GetBool(L"context.enabled", true))
    {
        return;
    }

    if (!m_context.commandDispatcher)
    {
        return;
    }

    if (command.fence.id.empty())
    {
        LogInfo(L"sort_this_fence skipped: no fence context payload");
        return;
    }

    CommandContext forward = command;
    forward.commandId = L"organizer.by_type";
    if (!m_context.commandDispatcher->Dispatch(L"organizer.by_type", forward))
    {
        LogInfo(L"sort_this_fence could not dispatch organizer.by_type");
        return;
    }

    Notify(L"Sort This Fence requested.");
}

void ContextActionsPlugin::HandleCleanupThisFence(const CommandContext& command) const
{
    if (!GetBool(L"context.enabled", true))
    {
        return;
    }

    if (!m_context.commandDispatcher)
    {
        return;
    }

    if (command.fence.id.empty())
    {
        LogInfo(L"cleanup_this_fence skipped: no fence context payload");
        return;
    }

    CommandContext forward = command;
    forward.commandId = L"organizer.cleanup_empty";
    if (!m_context.commandDispatcher->Dispatch(L"organizer.cleanup_empty", forward))
    {
        LogInfo(L"cleanup_this_fence could not dispatch organizer.cleanup_empty");
        return;
    }

    Notify(L"Clean Up This Fence requested.");
}

void ContextActionsPlugin::HandleApplyThemeThisFence(const CommandContext& command) const
{
    if (!GetBool(L"context.enabled", true))
    {
        return;
    }

    if (!m_context.commandDispatcher)
    {
        return;
    }

    if (command.fence.id.empty())
    {
        LogInfo(L"apply_theme_this_fence skipped: no fence context payload");
        return;
    }

    CommandContext forward = command;
    forward.commandId = L"appearance.mode.focus";
    if (!m_context.commandDispatcher->Dispatch(L"appearance.mode.focus", forward))
    {
        LogInfo(L"apply_theme_this_fence could not dispatch appearance.mode.focus");
        return;
    }

    Notify(L"Apply Focus Visual Mode requested.");
}

void ContextActionsPlugin::HandleRefreshProvider(const CommandContext& command) const
{
    if (!GetBool(L"context.enabled", true))
    {
        return;
    }

    if (!m_context.appCommands)
    {
        return;
    }

    if (command.fence.id.empty())
    {
        LogInfo(L"refresh_provider skipped: no fence context payload");
        return;
    }

    RefreshFenceWithThrottle(command.fence.id);
    LogInfo(L"refresh_provider refreshed fence: " + command.fence.id);
    Notify(L"Refresh Provider requested.");
}

void ContextActionsPlugin::HandlePinSelected(const CommandContext& command) const
{
    if (!GetBool(L"context.enabled", true))
    {
        return;
    }

    if (!command.item.has_value())
    {
        LogInfo(L"pin_selected skipped: no item context payload");
        return;
    }

    LogInfo(L"pin_selected requested for item='" + command.item->name + L"' (host pin API not exposed; sample logs intent)");
    Notify(L"Pin Selected requested.");
}

void ContextActionsPlugin::HandleCopyItemMetadata(const CommandContext& command) const
{
    if (!GetBool(L"context.enabled", true))
    {
        return;
    }

    if (!command.item.has_value())
    {
        LogInfo(L"copy_item_metadata skipped: no item context payload");
        return;
    }

    std::wstringstream summary;
    summary << L"item='" << command.item->name
            << L"' path='" << command.item->fullPath
            << L"' directory=" << (command.item->isDirectory ? L"true" : L"false");
    LogInfo(L"copy_item_metadata payload: " + summary.str());
    Notify(L"Item metadata captured.");
}

void ContextActionsPlugin::HandleOpenPluginSettings(const CommandContext&) const
{
    if (!GetBool(L"context.enabled", true))
    {
        return;
    }

    LogInfo(L"open_plugin_settings requested for plugin 'community.context_actions' (host deep-link API not exposed; sample logs intent)");
    Notify(L"Open Plugin Settings requested.");
}

bool ContextActionsPlugin::GetBool(const std::wstring& key, bool fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    return m_context.settingsRegistry->GetValue(key, fallback ? L"true" : L"false") == L"true";
}

int ContextActionsPlugin::GetInt(const std::wstring& key, int fallback) const
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

void ContextActionsPlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[ContextActions][Notification] " + message);
}

void ContextActionsPlugin::RefreshFenceWithThrottle(const std::wstring& fenceId) const
{
    if (!m_context.appCommands || fenceId.empty())
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
        LogInfo(L"refresh_provider throttled by plugin.refresh_interval_seconds");
        return;
    }

    m_lastRefreshAt = now;
    m_context.appCommands->RefreshFence(fenceId);
}

void ContextActionsPlugin::LogInfo(const std::wstring& message) const
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"[ContextActions] " + message);
    }
}


