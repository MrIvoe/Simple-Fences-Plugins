#include "ExternalProviderPlugin.h"

#include "extensions/FenceExtensionRegistry.h"
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
    m.displayName = L"External Provider Fences";
    m.version = L"1.0.1";
    m.description = L"Shows provider-backed virtual item lists from external and generated sources.";
    m.minHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.capabilities = {L"fence_content_provider", L"commands", L"settings_pages", L"tray_contributions"};
    return m;
}

bool ExternalProviderPlugin::Initialize(const PluginContext& context)
{
    m_context = context;
    if (!m_context.fenceExtensionRegistry || !m_context.commandDispatcher || !m_context.settingsRegistry || !m_context.appCommands)
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Error(L"ExternalProviderPlugin: Missing required host services");
        }
        return false;
    }

    FenceContentProviderDescriptor descriptor;
    descriptor.providerId = L"community.external_provider";
    descriptor.contentType = L"external_provider";
    descriptor.displayName = L"External Provider";
    descriptor.isCoreDefault = false;

    FenceContentProviderCallbacks callbacks;
    callbacks.enumerateItems = [this](const FenceMetadata& fence) {
        return EnumerateItems(fence);
    };
    callbacks.handleDrop = [this](const FenceMetadata& fence, const std::vector<std::wstring>& paths) {
        return HandleDrop(fence, paths);
    };
    callbacks.deleteItem = [this](const FenceMetadata& fence, const FenceItem& item) {
        return HandleDelete(fence, item);
    };

    m_context.fenceExtensionRegistry->RegisterContentProvider(descriptor, callbacks);

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
    page.fields.push_back(SettingsFieldDescriptor{L"provider.enabled", L"Enable external providers", L"Master toggle for external provider fences.", SettingsFieldType::Bool, L"true", {}, 10});
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

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"New External Provider Fence", L"provider.new", 710, false});
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

std::vector<FenceItem> ExternalProviderPlugin::EnumerateItems(const FenceMetadata& fence) const
{
    std::vector<FenceItem> items;
    if (!GetBool(L"provider.enabled", true))
    {
        m_context.appCommands->UpdateFenceContentState(fence.id, L"offline", L"Provider disabled by settings.");
        return items;
    }

    const bool cacheEnabled = GetBool(L"provider.cache_enabled", true);
    const int cacheTtl = GetInt(L"provider.cache_ttl_seconds", 600);
    if (cacheEnabled)
    {
        const auto it = m_cache.find(fence.id);
        if (it != m_cache.end())
        {
            const long long age = EpochSecondsNow() - it->second.timestampSeconds;
            if (age >= 0 && age <= cacheTtl)
            {
                m_context.appCommands->UpdateFenceContentState(fence.id, L"connected", L"Serving cached provider data.");
                return it->second.items;
            }
        }
    }

    const std::wstring source = ResolveSource(fence);
    if (source.empty())
    {
        m_context.appCommands->UpdateFenceContentState(fence.id, L"unavailable", L"No source configured.");
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

            FenceItem item;
            item.name = entry.path().filename().wstring();
            item.fullPath = entry.path().wstring();
            item.originalPath = entry.path().wstring();
            item.isDirectory = entry.is_directory();
            items.push_back(std::move(item));
        }

        m_context.appCommands->UpdateFenceContentState(fence.id, L"connected", L"Directory provider source loaded.");
    }
    else if (fs::exists(sourcePath, ec))
    {
        std::wifstream in(sourcePath);
        if (!in.is_open())
        {
            m_context.appCommands->UpdateFenceContentState(fence.id, L"permission_denied", L"Cannot open configured source file.");
            return items;
        }

        std::wstring line;
        while (std::getline(in, line))
        {
            if (line.empty())
            {
                continue;
            }

            FenceItem item;
            item.name = line;
            item.fullPath = line;
            item.originalPath = line;
            item.isDirectory = false;
            items.push_back(std::move(item));
        }

        m_context.appCommands->UpdateFenceContentState(fence.id, L"connected", L"File/list provider source loaded.");
    }
    else
    {
        m_context.appCommands->UpdateFenceContentState(fence.id, L"offline", L"Configured provider source path does not exist.");
        return items;
    }

    if (cacheEnabled)
    {
        m_cache[fence.id] = CacheEntry{items, EpochSecondsNow()};
    }

    return items;
}

bool ExternalProviderPlugin::HandleDrop(const FenceMetadata&, const std::vector<std::wstring>&) const
{
    return !GetBool(L"provider.read_only_default", true);
}

bool ExternalProviderPlugin::HandleDelete(const FenceMetadata&, const FenceItem&) const
{
    return !GetBool(L"provider.read_only_default", true);
}

void ExternalProviderPlugin::HandleProviderNew(const CommandContext&) const
{
    FenceCreateRequest request;
    request.title = L"External Provider";
    request.contentType = L"external_provider";
    request.contentPluginId = L"community.external_provider";
    request.contentSource = GetString(L"provider.json.source", L"");

    const std::wstring createdFence = m_context.appCommands->CreateFenceNearCursor(request);
    if (!createdFence.empty())
    {
        if (!request.contentSource.empty())
        {
            m_context.appCommands->UpdateFenceContentSource(createdFence, request.contentSource);
        }
        else
        {
            m_context.appCommands->UpdateFenceContentState(createdFence, L"unavailable", L"Set provider source in plugin settings.");
        }

        Notify(L"External provider fence created.");
    }
}

void ExternalProviderPlugin::HandleRefreshCurrent(const CommandContext& command) const
{
    const std::wstring fenceId = ResolveCurrentFenceId(command);
    if (fenceId.empty())
    {
        LogWarn(L"provider.refresh_current skipped: no fence context");
        return;
    }

    m_cache.erase(fenceId);
    RefreshFenceWithThrottle(fenceId);
    Notify(L"Refresh current provider requested.");
}

void ExternalProviderPlugin::HandleRefreshAll(const CommandContext&) const
{
    m_cache.clear();
    const auto ids = m_context.appCommands->GetAllFenceIds();
    bool any = false;
    for (const auto& id : ids)
    {
        const FenceMetadata fence = m_context.appCommands->GetFenceMetadata(id);
        if (fence.contentType == L"external_provider")
        {
            RefreshFenceWithThrottle(id);
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
    const auto ids = m_context.appCommands->GetAllFenceIds();
    bool any = false;
    for (const auto& id : ids)
    {
        const FenceMetadata fence = m_context.appCommands->GetFenceMetadata(id);
        if (fence.contentType != L"external_provider")
        {
            continue;
        }

        if (fence.contentState == L"offline" || fence.contentState == L"unavailable" || fence.contentState == L"permission_denied")
        {
            m_context.appCommands->UpdateFenceContentState(id, L"connected", L"Reconnect requested");
            m_cache.erase(id);
            RefreshFenceWithThrottle(id);
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

void ExternalProviderPlugin::RefreshFenceWithThrottle(const std::wstring& fenceId) const
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
        LogInfo(L"Provider refresh throttled by plugin.refresh_interval_seconds");
        return;
    }

    m_lastRefreshAtByFence[fenceId] = now;
    m_context.appCommands->RefreshFence(fenceId);
}

std::wstring ExternalProviderPlugin::ResolveSource(const FenceMetadata& fence) const
{
    if (!fence.contentSource.empty())
    {
        return fence.contentSource;
    }

    const std::wstring fromJson = GetString(L"provider.json.source", L"");
    if (!fromJson.empty())
    {
        return fromJson;
    }

    return GetString(L"provider.network.root_path", L"");
}

std::wstring ExternalProviderPlugin::ResolveCurrentFenceId(const CommandContext& command) const
{
    if (!command.fence.id.empty())
    {
        return command.fence.id;
    }

    const CommandContext current = m_context.appCommands->GetCurrentCommandContext();
    if (!current.fence.id.empty())
    {
        return current.fence.id;
    }

    return m_context.appCommands->GetActiveFenceMetadata().id;
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


