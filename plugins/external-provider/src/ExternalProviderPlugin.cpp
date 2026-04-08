#include "ExternalProviderPlugin.h"

#include "core/CommandDispatcher.h"
#include "core/Diagnostics.h"
#include "extensions/SpaceExtensionRegistry.h"
#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace
{
    long long EpochSecondsNow()
    {
        const auto now = std::chrono::system_clock::now();
        return static_cast<long long>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
    }
}

PluginManifest ExternalProviderPlugin::GetManifest() const
{
    PluginManifest m;
    m.id = L"community.external_provider";
    m.displayName = L"External Provider Spaces";
    m.version = L"1.0.2";
    m.description = L"Shows provider-backed virtual item lists from external and generated sources.";
    m.minHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.capabilities = {L"space_content_provider", L"commands", L"settings_pages", L"tray_contributions"};
    return m;
}

bool ExternalProviderPlugin::Initialize(const PluginContext& context)
{
    m_context = context;
    if (!m_context.spaceExtensionRegistry || !m_context.commandDispatcher || !m_context.settingsRegistry || !m_context.appCommands)
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Error(L"ExternalProviderPlugin: Missing required host services");
        }
        return false;
    }

    SpaceContentProviderDescriptor descriptor;
    descriptor.providerId = L"community.external_provider";
    descriptor.contentType = L"external_provider";
    descriptor.displayName = L"External Provider";
    descriptor.isCoreDefault = false;

    SpaceContentProviderCallbacks callbacks;
    callbacks.enumerateItems = [this](const SpaceMetadata& space) {
        return EnumerateItems(space);
    };
    callbacks.handleDrop = [this](const SpaceMetadata& space, const std::vector<std::wstring>& paths) {
        return HandleDrop(space, paths);
    };
    callbacks.deleteItem = [this](const SpaceMetadata& space, const SpaceItem& item) {
        return HandleDelete(space, item);
    };

    m_context.spaceExtensionRegistry->RegisterContentProvider(descriptor, callbacks);

    RegisterSettings();
    RegisterMenus();
    RegisterCommands();
    LogInfo(L"Initialized");
    return true;
}

void ExternalProviderPlugin::Shutdown()
{
    m_cache.clear();
    LogInfo(L"Shutdown");
}

void ExternalProviderPlugin::RegisterSettings() const
{
    PluginSettingsPage page;
    page.pluginId = L"community.external_provider";
    page.pageId = L"provider.general";
    page.title = L"External Provider";
    page.order = 80;

    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 60, false);
    page.fields.push_back(SettingsFieldDescriptor{L"provider.enabled", L"Enable external providers", L"Master toggle for external provider spaces.", SettingsFieldType::Bool, L"true", {}, 10});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.refresh_mode", L"Refresh mode", L"Choose manual, interval, startup, or hybrid refresh policy.", SettingsFieldType::Enum, L"hybrid", {{L"manual", L"Manual"}, {L"interval", L"Interval"}, {L"on_startup", L"On startup"}, {L"hybrid", L"Hybrid"}}, 20});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.refresh_interval_seconds", L"Refresh interval (s)", L"Interval for polling-based refresh.", SettingsFieldType::Int, L"300", {}, 30});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.timeout_seconds", L"Timeout (s)", L"Timeout budget for provider operations.", SettingsFieldType::Int, L"15", {}, 40});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.retry_count", L"Retry count", L"Retry attempts on transient errors.", SettingsFieldType::Int, L"3", {}, 50});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.cache_enabled", L"Enable cache", L"Cache provider results for repeated refreshes.", SettingsFieldType::Bool, L"true", {}, 60});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.cache_ttl_seconds", L"Cache TTL (s)", L"Maximum cache lifetime in seconds.", SettingsFieldType::Int, L"600", {}, 70});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.read_only_default", L"Read-only default", L"Prevent drop/delete operations unless disabled.", SettingsFieldType::Bool, L"true", {}, 80});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.rss.url", L"RSS URL", L"Optional RSS endpoint placeholder value.", SettingsFieldType::String, L"", {}, 90});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.json.source", L"JSON/list source path", L"Path to a local line-based source file.", SettingsFieldType::String, L"", {}, 100});
    page.fields.push_back(SettingsFieldDescriptor{L"provider.network.root_path", L"Network root path", L"Root path for network-backed listing.", SettingsFieldType::String, L"", {}, 110});

    m_context.settingsRegistry->RegisterPage(std::move(page));
}

void ExternalProviderPlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"New External Provider Space", L"provider.new", 710, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Refresh Current Provider", L"provider.refresh_current", 720, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Refresh All Providers", L"provider.refresh_all", 730, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Reconnect Failed Providers", L"provider.reconnect_failed", 740, false});
}

void ExternalProviderPlugin::RegisterCommands() const
{
    m_context.commandDispatcher->RegisterCommand(L"provider.new", [this](const CommandContext& command) { HandleProviderNew(command); });
    m_context.commandDispatcher->RegisterCommand(L"provider.refresh_current", [this](const CommandContext& command) { HandleRefreshCurrent(command); });
    m_context.commandDispatcher->RegisterCommand(L"provider.refresh_all", [this](const CommandContext& command) { HandleRefreshAll(command); });
    m_context.commandDispatcher->RegisterCommand(L"provider.reconnect_failed", [this](const CommandContext& command) { HandleReconnectFailed(command); });
}

std::vector<SpaceItem> ExternalProviderPlugin::EnumerateItems(const SpaceMetadata& space) const
{
    std::vector<SpaceItem> items;
    if (!GetBool(L"provider.enabled", true))
    {
        m_context.appCommands->UpdateSpaceContentState(space.id, L"offline", L"Provider disabled by settings.");
        return items;
    }

    const bool cacheEnabled = GetBool(L"provider.cache_enabled", true);
    const int cacheTtl = GetInt(L"provider.cache_ttl_seconds", 600);
    if (cacheEnabled)
    {
        const auto it = m_cache.find(space.id);
        if (it != m_cache.end())
        {
            const long long age = EpochSecondsNow() - it->second.timestampSeconds;
            if (age >= 0 && age <= cacheTtl)
            {
                m_context.appCommands->UpdateSpaceContentState(space.id, L"connected", L"Serving cached provider data.");
                return it->second.items;
            }
        }
    }

    const std::wstring source = ResolveSource(space);
    if (source.empty())
    {
        m_context.appCommands->UpdateSpaceContentState(space.id, L"unavailable", L"No source configured.");
        return items;
    }

    std::error_code ec;
    const fs::path sourcePath(source);
    if (fs::exists(sourcePath, ec) && fs::is_directory(sourcePath, ec))
    {
        for (const auto& entry : fs::directory_iterator(sourcePath, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }

            SpaceItem item;
            item.name = entry.path().filename().wstring();
            item.fullPath = entry.path().wstring();
            item.originalPath = entry.path().wstring();
            item.isDirectory = entry.is_directory();
            items.push_back(std::move(item));
        }

        m_context.appCommands->UpdateSpaceContentState(space.id, L"connected", L"Directory provider source loaded.");
    }
    else if (fs::exists(sourcePath, ec))
    {
        std::wifstream in(sourcePath);
        if (!in.is_open())
        {
            m_context.appCommands->UpdateSpaceContentState(space.id, L"permission_denied", L"Cannot open configured source file.");
            return items;
        }

        std::wstring line;
        while (std::getline(in, line))
        {
            if (line.empty())
            {
                continue;
            }

            SpaceItem item;
            item.name = line;
            item.fullPath = line;
            item.originalPath = line;
            item.isDirectory = false;
            items.push_back(std::move(item));
        }

        m_context.appCommands->UpdateSpaceContentState(space.id, L"connected", L"File/list provider source loaded.");
    }
    else
    {
        m_context.appCommands->UpdateSpaceContentState(space.id, L"offline", L"Configured provider source path does not exist.");
        return items;
    }

    if (cacheEnabled)
    {
        m_cache[space.id] = CacheEntry{items, EpochSecondsNow()};
    }

    return items;
}

bool ExternalProviderPlugin::HandleDrop(const SpaceMetadata&, const std::vector<std::wstring>&) const
{
    return !GetBool(L"provider.read_only_default", true);
}

bool ExternalProviderPlugin::HandleDelete(const SpaceMetadata&, const SpaceItem&) const
{
    return !GetBool(L"provider.read_only_default", true);
}

void ExternalProviderPlugin::HandleProviderNew(const CommandContext&) const
{
    SpaceCreateRequest request;
    request.title = L"External Provider";
    request.contentType = L"external_provider";
    request.contentPluginId = L"community.external_provider";
    request.contentSource = GetString(L"provider.json.source", L"");

    const std::wstring createdSpace = m_context.appCommands->CreateSpaceNearCursor(request);
    if (!createdSpace.empty())
    {
        if (!request.contentSource.empty())
        {
            m_context.appCommands->UpdateSpaceContentSource(createdSpace, request.contentSource);
        }
        else
        {
            m_context.appCommands->UpdateSpaceContentState(createdSpace, L"unavailable", L"Set provider source in plugin settings.");
        }

        Notify(L"External provider space created.");
    }
}

void ExternalProviderPlugin::HandleRefreshCurrent(const CommandContext& command) const
{
    const std::wstring spaceId = ResolveCurrentSpaceId(command);
    if (spaceId.empty())
    {
        LogWarn(L"provider.refresh_current skipped: no space context");
        return;
    }

    m_cache.erase(spaceId);
    RefreshSpaceWithThrottle(spaceId);
    Notify(L"Refresh current provider requested.");
}

void ExternalProviderPlugin::HandleRefreshAll(const CommandContext&) const
{
    m_cache.clear();
    const auto ids = m_context.appCommands->GetAllSpaceIds();
    bool any = false;
    for (const auto& id : ids)
    {
        const SpaceMetadata space = m_context.appCommands->GetSpaceMetadata(id);
        if (space.contentType == L"external_provider")
        {
            RefreshSpaceWithThrottle(id);
            any = true;
        }
    }

    if (any)
    {
        Notify(L"Refresh all providers requested.");
    }
}

void ExternalProviderPlugin::HandleReconnectFailed(const CommandContext&) const
{
    const auto ids = m_context.appCommands->GetAllSpaceIds();
    bool any = false;
    for (const auto& id : ids)
    {
        const SpaceMetadata space = m_context.appCommands->GetSpaceMetadata(id);
        if (space.contentType != L"external_provider")
        {
            continue;
        }

        if (space.contentState == L"offline" || space.contentState == L"unavailable" || space.contentState == L"permission_denied")
        {
            m_context.appCommands->UpdateSpaceContentState(id, L"connected", L"Reconnect requested");
            m_cache.erase(id);
            RefreshSpaceWithThrottle(id);
            any = true;
        }
    }

    if (any)
    {
        Notify(L"Reconnect failed providers requested.");
    }
}

bool ExternalProviderPlugin::GetBool(const std::wstring& key, bool fallback) const
{
    return GetString(key, fallback ? L"true" : L"false") == L"true";
}

int ExternalProviderPlugin::GetInt(const std::wstring& key, int fallback) const
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

std::wstring ExternalProviderPlugin::GetString(const std::wstring& key, const std::wstring& fallback) const
{
    return m_context.settingsRegistry ? m_context.settingsRegistry->GetValue(key, fallback) : fallback;
}

void ExternalProviderPlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[ExternalProvider][Notification] " + message);
}

void ExternalProviderPlugin::RefreshSpaceWithThrottle(const std::wstring& spaceId) const
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
        LogInfo(L"Provider refresh throttled by plugin.refresh_interval_seconds");
        return;
    }

    m_lastRefreshAtBySpace[spaceId] = now;
    m_context.appCommands->RefreshSpace(spaceId);
}

std::wstring ExternalProviderPlugin::ResolveSource(const SpaceMetadata& space) const
{
    if (!space.contentSource.empty())
    {
        return space.contentSource;
    }

    const std::wstring fromJson = GetString(L"provider.json.source", L"");
    if (!fromJson.empty())
    {
        return fromJson;
    }

    return GetString(L"provider.network.root_path", L"");
}

std::wstring ExternalProviderPlugin::ResolveCurrentSpaceId(const CommandContext& command) const
{
    if (!command.space.id.empty())
    {
        return command.space.id;
    }

    const CommandContext current = m_context.appCommands->GetCurrentCommandContext();
    if (!current.space.id.empty())
    {
        return current.space.id;
    }

    return m_context.appCommands->GetActiveSpaceMetadata().id;
}

void ExternalProviderPlugin::LogInfo(const std::wstring& message) const
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Info(L"[ExternalProvider] " + message);
    }
}

void ExternalProviderPlugin::LogWarn(const std::wstring& message) const
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Warn(L"[ExternalProvider] " + message);
    }
}



