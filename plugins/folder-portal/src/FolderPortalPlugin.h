#pragma once

#include "Models.h"
#include "extensions/PluginContracts.h"

#include <chrono>
#include <unordered_map>
#include <vector>

// Community plugin: Folder Portal
// Registers a folder_portal content provider and operational settings pages.
// Capability: space_content_provider, commands, tray_contributions, settings_pages
class FolderPortalPlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    PluginContext m_context{};
    mutable std::unordered_map<std::wstring, std::chrono::steady_clock::time_point> m_lastRefreshAtBySpace;

    void RegisterSettings() const;
    void RegisterMenus() const;
    void RegisterCommands() const;

    std::vector<SpaceItem> EnumeratePortalItems(const SpaceMetadata& space) const;
    bool HandlePortalDrop(const SpaceMetadata& space, const std::vector<std::wstring>& paths) const;
    bool HandlePortalDelete(const SpaceMetadata& space, const SpaceItem& item) const;

    void HandleNewPortal(const CommandContext& command) const;
    void HandleReconnectAll(const CommandContext& command) const;
    void HandleRefreshAll(const CommandContext& command) const;
    void HandlePauseUpdatesToggle(const CommandContext& command) const;
    void HandleOpenPortalSource(const CommandContext& command) const;
    void HandleConvertToStatic(const CommandContext& command) const;

    bool GetBool(const std::wstring& key, bool fallback) const;
    int GetInt(const std::wstring& key, int fallback) const;
    void Notify(const std::wstring& message) const;
    void RefreshSpaceWithThrottle(const std::wstring& spaceId) const;
    void UpdatePortalHealth(const SpaceMetadata& space) const;
};
