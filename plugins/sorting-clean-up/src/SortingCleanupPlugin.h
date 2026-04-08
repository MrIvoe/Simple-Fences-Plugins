#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>
#include <filesystem>

class SortingCleanupPlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    PluginContext m_context{};
    mutable bool m_autoSortEnabled = false;
    mutable std::chrono::steady_clock::time_point m_lastRefreshAt{};

    void RegisterSettings() const;
    void RegisterMenus() const;
    void RegisterCommands() const;

    void HandleSortByName(const CommandContext& command) const;
    void HandleSortByType(const CommandContext& command) const;
    void HandleSortByModified(const CommandContext& command) const;
    void HandleCleanupCurrent(const CommandContext& command) const;
    void HandleAlignGrid(const CommandContext& command) const;
    void HandleAutoSortToggle(const CommandContext& command) const;

    SpaceMetadata ResolveSpace(const CommandContext& command) const;
    bool GetBool(const std::wstring& key, bool fallback) const;
    int GetInt(const std::wstring& key, int fallback) const;
    std::wstring GetString(const std::wstring& key, const std::wstring& fallback) const;
    bool IsPluginEnabled() const;
    bool ShouldLogActions() const;
    bool IsSafeModeEnabled() const;
    std::wstring GetDefaultMode() const;
    int GetRefreshIntervalSeconds() const;
    void Notify(const std::wstring& message) const;
    void RefreshSpaceWithThrottle(const std::wstring& spaceId) const;
    void SetSetting(const std::wstring& key, const std::wstring& value) const;
    void LogInfo(const std::wstring& message) const;
    void LogWarn(const std::wstring& message) const;

    static std::filesystem::path BuildUniquePath(const std::filesystem::path& target);
    void ApplySortPlan(const SpaceMetadata& space, const std::vector<std::filesystem::path>& files) const;
};
