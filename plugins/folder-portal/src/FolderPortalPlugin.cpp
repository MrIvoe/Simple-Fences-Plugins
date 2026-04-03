#include "FolderPortalPlugin.h"

#include "../../shared/SampleFsHelpers.h"
#include "core/Diagnostics.h"
#include "extensions/FenceExtensionRegistry.h"
#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <shellapi.h>
#endif

namespace fs = std::filesystem;

PluginManifest FolderPortalPlugin::GetManifest() const
{
    PluginManifest m;
    m.id = L"community.folder_portal";
    m.displayName = L"Folder Portal";
    m.version = L"1.2.1";
    m.description = L"Fence content provider that mirrors an existing source folder with health-aware state updates.";
    m.minHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.capabilities = {L"fence_content_provider", L"commands", L"menu_contributions", L"tray_contributions", L"settings_pages"};
    return m;
}

bool FolderPortalPlugin::Initialize(const PluginContext& context)
{
    m_context = context;

    if (m_context.fenceExtensionRegistry)
    {
        FenceContentProviderDescriptor provider;
        provider.providerId = L"community.folder_portal";
        provider.contentType = L"folder_portal";
        provider.displayName = L"Folder Portal";
        provider.isCoreDefault = false;

        FenceContentProviderCallbacks callbacks;
        callbacks.enumerateItems = [this](const FenceMetadata& fence) {
            return EnumeratePortalItems(fence);
        };
        callbacks.handleDrop = [this](const FenceMetadata& fence, const std::vector<std::wstring>& paths) {
            return HandlePortalDrop(fence, paths);
        };
        callbacks.deleteItem = [this](const FenceMetadata& fence, const FenceItem& item) {
            return HandlePortalDelete(fence, item);
        };

        m_context.fenceExtensionRegistry->RegisterContentProvider(provider, callbacks);
    }

    RegisterSettings();
    RegisterMenus();
    RegisterCommands();

    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"FolderPortalPlugin initialized");
    }

    return true;
}

void FolderPortalPlugin::Shutdown()
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"FolderPortalPlugin shutdown");
    }
}

bool FolderPortalPlugin::GetBool(const std::wstring& key, bool fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    return m_context.settingsRegistry->GetValue(key, fallback ? L"true" : L"false") == L"true";
}

int FolderPortalPlugin::GetInt(const std::wstring& key, int fallback) const
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

void FolderPortalPlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[FolderPortal][Notification] " + message);
}

void FolderPortalPlugin::RefreshFenceWithThrottle(const std::wstring& fenceId) const
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
    const auto it = m_lastRefreshAtByFence.find(fenceId);
    if (it != m_lastRefreshAtByFence.end() && (now - it->second) < std::chrono::seconds(seconds))
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Info(L"FolderPortal: refresh throttled by plugin.refresh_interval_seconds");
        }
        return;
    }

    m_lastRefreshAtByFence[fenceId] = now;
    m_context.appCommands->RefreshFence(fenceId);
}

void FolderPortalPlugin::RegisterSettings() const
{
    if (!m_context.settingsRegistry)
    {
        return;
    }

    PluginSettingsPage general;
    general.pluginId = L"community.folder_portal";
    general.pageId = L"portal.general";
    general.title = L"General";
    general.order = 10;

    PluginUiPatterns::AppendBaselineSettingsFields(general.fields, 1, 60, false);
    general.fields.push_back(SettingsFieldDescriptor{L"portal.general.enabled", L"Enable folder portals", L"Master toggle for folder portal behavior.", SettingsFieldType::Bool, L"true", {}, 10});
    general.fields.push_back(SettingsFieldDescriptor{L"portal.general.default_mode", L"Default portal mode", L"Select how portal drag and drop is handled.", SettingsFieldType::Enum, L"read_only", {{L"read_only", L"Read-only"}, {L"copy_in", L"Copy into source"}, {L"move_in", L"Move into source"}}, 20});
    general.fields.push_back(SettingsFieldDescriptor{L"portal.general.allow_open", L"Allow open source", L"Allow opening portal source folder from command surfaces.", SettingsFieldType::Bool, L"true", {}, 25});
    general.fields.push_back(SettingsFieldDescriptor{L"portal.general.allow_rename", L"Allow rename", L"Allow rename operations for portal items where host supports it.", SettingsFieldType::Bool, L"false", {}, 28});
    general.fields.push_back(SettingsFieldDescriptor{L"portal.general.allow_delete", L"Allow delete", L"Allow deleting source items from portal fences.", SettingsFieldType::Bool, L"false", {}, 30});
    general.fields.push_back(SettingsFieldDescriptor{L"portal.general.show_hidden", L"Show hidden files", L"Include hidden files in portal enumeration.", SettingsFieldType::Bool, L"false", {}, 35});
    general.fields.push_back(SettingsFieldDescriptor{L"portal.general.show_system", L"Show system files", L"Include system files in portal enumeration where detectable.", SettingsFieldType::Bool, L"false", {}, 36});
    general.fields.push_back(SettingsFieldDescriptor{L"portal.general.new_portal_default_source", L"New portal default source", L"Optional default path for portal.new command.", SettingsFieldType::String, L"", {}, 40});

    m_context.settingsRegistry->RegisterPage(std::move(general));

    PluginSettingsPage watch;
    watch.pluginId = L"community.folder_portal";
    watch.pageId = L"portal.watch";
    watch.title = L"Watching";
    watch.order = 20;

    watch.fields.push_back(SettingsFieldDescriptor{L"portal.watch.enabled_live_refresh", L"Enable live refresh", L"Enable live refresh watch behavior where host supports it.", SettingsFieldType::Bool, L"true", {}, 5});
    watch.fields.push_back(SettingsFieldDescriptor{L"portal.watch.debounce_ms", L"Debounce (ms)", L"Debounce interval for live refresh update bursts.", SettingsFieldType::Int, L"400", {}, 8});
    watch.fields.push_back(SettingsFieldDescriptor{L"portal.watch.rescan_interval_seconds", L"Rescan interval (s)", L"Periodic full refresh interval.", SettingsFieldType::Int, L"60", {}, 9});
    watch.fields.push_back(SettingsFieldDescriptor{L"portal.watch.recurse_subfolders", L"Include subfolders", L"Include subfolder items in portal enumeration.", SettingsFieldType::Bool, L"false", {}, 10});
    watch.fields.push_back(SettingsFieldDescriptor{L"portal.watch.pause_while_dragging", L"Pause while dragging", L"Pause watch refresh while drag operations are active.", SettingsFieldType::Bool, L"true", {}, 18});
    watch.fields.push_back(SettingsFieldDescriptor{L"portal.watch.show_hidden", L"Show hidden files (legacy)", L"Legacy compatibility key for hidden-file visibility.", SettingsFieldType::Bool, L"false", {}, 20});
    watch.fields.push_back(SettingsFieldDescriptor{L"portal.watch.retry_unavailable", L"Retry unavailable portals", L"When true, reconnect_all sets disconnected portals back to connecting and refreshes them.", SettingsFieldType::Bool, L"true", {}, 30});
    watch.fields.push_back(SettingsFieldDescriptor{L"portal.watch.retry_interval_seconds", L"Retry interval (s)", L"Retry cadence for unavailable portal sources.", SettingsFieldType::Int, L"30", {}, 35});

    m_context.settingsRegistry->RegisterPage(std::move(watch));

    PluginSettingsPage safety;
    safety.pluginId = L"community.folder_portal";
    safety.pageId = L"portal.safety";
    safety.title = L"Safety";
    safety.order = 30;

    safety.fields.push_back(SettingsFieldDescriptor{L"portal.safety.read_only_network_default", L"Read-only network default", L"Force read-only behavior for network paths by default.", SettingsFieldType::Bool, L"true", {}, 5});
    safety.fields.push_back(SettingsFieldDescriptor{L"portal.safety.warn_destructive", L"Warn destructive actions", L"Warn before destructive source actions where supported.", SettingsFieldType::Bool, L"true", {}, 7});
    safety.fields.push_back(SettingsFieldDescriptor{L"portal.safety.block_drop_readonly", L"Block drops when read-only", L"Block drop operations when in read-only mode.", SettingsFieldType::Bool, L"true", {}, 8});
    safety.fields.push_back(SettingsFieldDescriptor{L"portal.safety.keep_visible_when_missing", L"Keep visible when source missing", L"When source folder is missing, keep the fence and mark it disconnected.", SettingsFieldType::Bool, L"true", {}, 10});
    m_context.settingsRegistry->RegisterPage(std::move(safety));

    PluginSettingsPage display;
    display.pluginId = L"community.folder_portal";
    display.pageId = L"portal.display";
    display.title = L"Display";
    display.order = 40;

    display.fields.push_back(SettingsFieldDescriptor{L"portal.display.show_path_tooltip", L"Show path tooltip", L"Show source path tooltip in portal UI surfaces.", SettingsFieldType::Bool, L"true", {}, 10});
    display.fields.push_back(SettingsFieldDescriptor{L"portal.display.show_health_badge", L"Show health badge", L"Show health state badge in portal UI surfaces.", SettingsFieldType::Bool, L"true", {}, 20});
    m_context.settingsRegistry->RegisterPage(std::move(display));
}

void FolderPortalPlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"New Folder Portal", L"portal.new", 110, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Reconnect All Portals", L"portal.reconnect_all", 120, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Refresh All Portals", L"portal.refresh_all", 130, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Pause Portal Updates", L"portal.pause_updates_toggle", 140, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Convert Portal to Static", L"portal.convert_to_static", 150, false});

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::FenceContext, L"Open Portal Source Folder", L"portal.open_source", 230, true});
}

void FolderPortalPlugin::RegisterCommands() const
{
    if (!m_context.commandDispatcher)
    {
        return;
    }

    m_context.commandDispatcher->RegisterCommand(L"portal.new", [this](const CommandContext& command) { HandleNewPortal(command); });
    m_context.commandDispatcher->RegisterCommand(L"portal.reconnect_all", [this](const CommandContext& command) { HandleReconnectAll(command); });
    m_context.commandDispatcher->RegisterCommand(L"portal.refresh_all", [this](const CommandContext& command) { HandleRefreshAll(command); });
    m_context.commandDispatcher->RegisterCommand(L"portal.pause_updates_toggle", [this](const CommandContext& command) { HandlePauseUpdatesToggle(command); });
    m_context.commandDispatcher->RegisterCommand(L"portal.open_source", [this](const CommandContext& command) { HandleOpenPortalSource(command); });
    m_context.commandDispatcher->RegisterCommand(L"portal.convert_to_static", [this](const CommandContext& command) { HandleConvertToStatic(command); });
}

std::vector<FenceItem> FolderPortalPlugin::EnumeratePortalItems(const FenceMetadata& fence) const
{
    std::vector<FenceItem> items;
    if (!m_context.settingsRegistry || !m_context.appCommands)
    {
        return items;
    }

    const bool recurse = m_context.settingsRegistry->GetValue(L"portal.watch.recurse_subfolders", L"false") == L"true";
    const bool showHiddenGeneral = m_context.settingsRegistry->GetValue(L"portal.general.show_hidden", L"false") == L"true";
    const bool showHiddenLegacy = m_context.settingsRegistry->GetValue(L"portal.watch.show_hidden", L"false") == L"true";
    const bool showHidden = showHiddenGeneral || showHiddenLegacy;

    fs::path source(fence.contentSource);
    std::error_code ec;
    if (source.empty() || !fs::exists(source, ec) || !fs::is_directory(source, ec))
    {
        m_context.appCommands->UpdateFenceContentState(fence.id, L"disconnected", L"Portal source is unavailable.");
        return items;
    }

    if (recurse)
    {
        for (const auto& entry : fs::recursive_directory_iterator(source, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }

            if (!showHidden)
            {
                if (SampleFsHelpers::IsHiddenPath(entry.path()))
                {
                    continue;
                }
            }

            FenceItem item;
            item.name = entry.path().filename().wstring();
            item.fullPath = entry.path().wstring();
            item.originalPath = entry.path().wstring();
            item.isDirectory = entry.is_directory();
            items.push_back(std::move(item));
        }
    }
    else
    {
        for (const auto& entry : fs::directory_iterator(source, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }

            if (!showHidden)
            {
                if (SampleFsHelpers::IsHiddenPath(entry.path()))
                {
                    continue;
                }
            }

            FenceItem item;
            item.name = entry.path().filename().wstring();
            item.fullPath = entry.path().wstring();
            item.originalPath = entry.path().wstring();
            item.isDirectory = entry.is_directory();
            items.push_back(std::move(item));
        }
    }

    m_context.appCommands->UpdateFenceContentState(fence.id, L"ready", L"");
    return items;
}

bool FolderPortalPlugin::HandlePortalDrop(const FenceMetadata& fence, const std::vector<std::wstring>& paths) const
{
    if (!m_context.settingsRegistry)
    {
        return false;
    }

    std::wstring mode = m_context.settingsRegistry->GetValue(L"portal.general.default_mode", L"read_only");
    const bool blockDropReadOnly = m_context.settingsRegistry->GetValue(L"portal.safety.block_drop_readonly", L"true") == L"true";
    const bool readOnlyNetworkDefault = m_context.settingsRegistry->GetValue(L"portal.safety.read_only_network_default", L"true") == L"true";
    if (readOnlyNetworkDefault)
    {
        const fs::path sourceRoot(fence.contentSource);
        const std::wstring native = sourceRoot.native();
        if (native.rfind(L"\\\\", 0) == 0)
        {
            mode = L"read_only";
        }
    }

    if (mode == L"read_only" && blockDropReadOnly)
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Info(L"FolderPortal: drop ignored in read-only mode");
        }
        return false;
    }

    std::error_code ec;
    const fs::path sourceRoot(fence.contentSource);
    if (sourceRoot.empty() || !fs::exists(sourceRoot, ec) || !fs::is_directory(sourceRoot, ec))
    {
        return false;
    }

    bool movedAny = false;
    for (const auto& raw : paths)
    {
        if (raw.empty())
        {
            continue;
        }

        fs::path source(raw);
        fs::path target = SampleFsHelpers::BuildUniquePath(sourceRoot / source.filename());

        ec.clear();
        if (mode == L"copy_in")
        {
            fs::copy(source, target, fs::copy_options::recursive, ec);
        }
        else
        {
            fs::rename(source, target, ec);
            if (ec)
            {
                ec.clear();
                fs::copy(source, target, fs::copy_options::recursive, ec);
                if (!ec)
                {
                    if (fs::is_directory(source, ec))
                    {
                        fs::remove_all(source, ec);
                    }
                    else
                    {
                        fs::remove(source, ec);
                    }
                }
            }
        }

        if (!ec)
        {
            movedAny = true;
        }
    }

    if (movedAny && m_context.appCommands)
    {
        m_context.appCommands->UpdateFenceContentState(fence.id, L"ready", L"");
    }

    return movedAny;
}

bool FolderPortalPlugin::HandlePortalDelete(const FenceMetadata& fence, const FenceItem& item) const
{
    if (!m_context.settingsRegistry)
    {
        return false;
    }

    if (m_context.settingsRegistry->GetValue(L"portal.general.allow_delete", L"false") != L"true")
    {
        return false;
    }

    std::error_code ec;
    const fs::path target(item.fullPath);
    if (item.isDirectory)
    {
        fs::remove_all(target, ec);
    }
    else
    {
        fs::remove(target, ec);
    }

    if (ec)
    {
        return false;
    }

    if (m_context.appCommands)
    {
        m_context.appCommands->UpdateFenceContentState(fence.id, L"ready", L"");
    }

    return true;
}

void FolderPortalPlugin::HandleNewPortal(const CommandContext&) const
{
    if (!m_context.appCommands)
    {
        return;
    }

    FenceCreateRequest request;
    request.title = L"Folder Portal";
    request.contentType = L"folder_portal";
    request.contentPluginId = L"community.folder_portal";

    if (m_context.settingsRegistry)
    {
        request.contentSource = m_context.settingsRegistry->GetValue(L"portal.general.new_portal_default_source", L"");
    }

    const std::wstring createdFenceId = m_context.appCommands->CreateFenceNearCursor(request);
    if (createdFenceId.empty())
    {
        return;
    }

    if (request.contentSource.empty())
    {
        m_context.appCommands->UpdateFenceContentState(createdFenceId, L"needs_source", L"Set a source path from host portal setup flow.");
    }
    else
    {
        m_context.appCommands->UpdateFenceContentSource(createdFenceId, request.contentSource);
    }
}

void FolderPortalPlugin::HandleReconnectAll(const CommandContext&) const
{
    if (!m_context.appCommands || !m_context.settingsRegistry)
    {
        return;
    }

    if (m_context.settingsRegistry->GetValue(L"portal.watch.retry_unavailable", L"true") != L"true")
    {
        return;
    }

    int retryIntervalSeconds = 30;
    try
    {
        retryIntervalSeconds = std::max(1, std::stoi(m_context.settingsRegistry->GetValue(L"portal.watch.retry_interval_seconds", L"30")));
    }
    catch (...)
    {
        retryIntervalSeconds = 30;
    }
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"FolderPortal: reconnect_all requested with retry_interval_seconds=" + std::to_wstring(retryIntervalSeconds));
    }

    const auto fenceIds = m_context.appCommands->GetAllFenceIds();
    bool any = false;
    for (const auto& fenceId : fenceIds)
    {
        const FenceMetadata fence = m_context.appCommands->GetFenceMetadata(fenceId);
        if (fence.contentType == L"folder_portal")
        {
            m_context.appCommands->UpdateFenceContentState(fence.id, L"connecting", L"Reconnect requested.");
            RefreshFenceWithThrottle(fence.id);
            UpdatePortalHealth(fence);
            any = true;
        }
    }

    if (any)
    {
        Notify(L"Reconnect all portals requested.");
    }
}

void FolderPortalPlugin::HandleRefreshAll(const CommandContext&) const
{
    if (!m_context.appCommands)
    {
        return;
    }

    const auto fenceIds = m_context.appCommands->GetAllFenceIds();
    bool any = false;
    for (const auto& fenceId : fenceIds)
    {
        const FenceMetadata fence = m_context.appCommands->GetFenceMetadata(fenceId);
        if (fence.contentType == L"folder_portal")
        {
            RefreshFenceWithThrottle(fence.id);
            UpdatePortalHealth(fence);
            any = true;
        }
    }

    if (any)
    {
        Notify(L"Refresh all portals requested.");
    }
}

void FolderPortalPlugin::HandlePauseUpdatesToggle(const CommandContext&) const
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"portal.pause_updates_toggle requested (sample plugin logs only; host owns watcher lifecycle)");
    }
    Notify(L"Pause portal updates toggle requested.");
}

void FolderPortalPlugin::HandleOpenPortalSource(const CommandContext& command) const
{
#ifdef _WIN32
    if (m_context.settingsRegistry && m_context.settingsRegistry->GetValue(L"portal.general.allow_open", L"true") != L"true")
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Info(L"FolderPortal: open_source skipped because portal.general.allow_open=false");
        }
        return;
    }

    std::wstring source = command.fence.contentSource;
    if (source.empty() && m_context.appCommands)
    {
        source = m_context.appCommands->GetCurrentCommandContext().fence.contentSource;
    }

    if (!source.empty())
    {
        ShellExecuteW(nullptr, L"open", source.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        Notify(L"Open portal source requested.");
    }
#else
    (void)command;
#endif
}

void FolderPortalPlugin::HandleConvertToStatic(const CommandContext& command) const
{
    if (!m_context.appCommands)
    {
        return;
    }

    std::wstring fenceId = command.fence.id;
    if (fenceId.empty())
    {
        fenceId = m_context.appCommands->GetCurrentCommandContext().fence.id;
    }

    if (fenceId.empty())
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Warn(L"FolderPortal: convert_to_static skipped because no fence context was available");
        }
        return;
    }

    // Host contract does not yet expose a content-type mutation API, so this command
    // marks state and logs intent while preserving current source-linked behavior.
    m_context.appCommands->UpdateFenceContentState(fenceId, L"ready", L"Convert-to-static requested (host content conversion API pending).");
    RefreshFenceWithThrottle(fenceId);
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"FolderPortal: convert_to_static requested for fence " + fenceId);
    }
    Notify(L"Convert portal to static requested.");
}

void FolderPortalPlugin::UpdatePortalHealth(const FenceMetadata& fence) const
{
    if (!m_context.appCommands)
    {
        return;
    }

    std::error_code ec;
    const fs::path source(fence.contentSource);
    if (source.empty())
    {
        m_context.appCommands->UpdateFenceContentState(fence.id, L"needs_source", L"Portal source path is not set.");
        return;
    }

    if (!fs::exists(source, ec) || !fs::is_directory(source, ec))
    {
        m_context.appCommands->UpdateFenceContentState(fence.id, L"disconnected", L"Portal source is unavailable.");
        return;
    }

    m_context.appCommands->UpdateFenceContentState(fence.id, L"ready", L"");
}

