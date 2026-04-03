#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>
#include <unordered_map>

// Community plugin: Folder Portal
// Registers a folder_portal content provider and operational settings pages.
// Capability: fence_content_provider, commands, tray_contributions, settings_pages
class FolderPortalPlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    PluginContext m_context{};
    mutable std::unordered_map<std::wstring, std::chrono::steady_clock::time_point> m_lastRefreshAtByFence;

    void RegisterSettings() const;
    void RegisterMenus() const;
    void RegisterCommands() const;

    std::vector<FenceItem> EnumeratePortalItems(const FenceMetadata& fence) const;
    bool HandlePortalDrop(const FenceMetadata& fence, const std::vector<std::wstring>& paths) const;
    bool HandlePortalDelete(const FenceMetadata& fence, const FenceItem& item) const;

    void HandleNewPortal(const CommandContext& command) const;
    void HandleReconnectAll(const CommandContext& command) const;
    void HandleRefreshAll(const CommandContext& command) const;
    void HandlePauseUpdatesToggle(const CommandContext& command) const;
    void HandleOpenPortalSource(const CommandContext& command) const;
    void HandleConvertToStatic(const CommandContext& command) const;

    bool GetBool(const std::wstring& key, bool fallback) const;
    int GetInt(const std::wstring& key, int fallback) const;
    void Notify(const std::wstring& message) const;
    void RefreshFenceWithThrottle(const std::wstring& fenceId) const;
    void UpdatePortalHealth(const FenceMetadata& fence) const;
};
