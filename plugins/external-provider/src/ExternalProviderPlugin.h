#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>
#include <unordered_map>
#include <vector>

class ExternalProviderPlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    struct CacheEntry
    {
        std::vector<FenceItem> items;
        long long timestampSeconds = 0;
    };

    PluginContext m_context{};
    mutable std::unordered_map<std::wstring, CacheEntry> m_cache;
    mutable std::unordered_map<std::wstring, std::chrono::steady_clock::time_point> m_lastRefreshAtByFence;

    void RegisterSettings() const;
    void RegisterMenus() const;
    void RegisterCommands() const;

    std::vector<FenceItem> EnumerateItems(const FenceMetadata& fence) const;
    bool HandleDrop(const FenceMetadata& fence, const std::vector<std::wstring>& paths) const;
    bool HandleDelete(const FenceMetadata& fence, const FenceItem& item) const;

    void HandleProviderNew(const CommandContext& command) const;
    void HandleRefreshCurrent(const CommandContext& command) const;
    void HandleRefreshAll(const CommandContext& command) const;
    void HandleReconnectFailed(const CommandContext& command) const;

    bool GetBool(const std::wstring& key, bool fallback) const;
    int GetInt(const std::wstring& key, int fallback) const;
    std::wstring GetString(const std::wstring& key, const std::wstring& fallback) const;
    void Notify(const std::wstring& message) const;
    void RefreshFenceWithThrottle(const std::wstring& fenceId) const;
    std::wstring ResolveSource(const FenceMetadata& fence) const;
    std::wstring ResolveCurrentFenceId(const CommandContext& command) const;
    void LogInfo(const std::wstring& message) const;
    void LogWarn(const std::wstring& message) const;
};
