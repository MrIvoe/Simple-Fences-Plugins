#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>
#include <filesystem>

// Community plugin: Space Organizer
// Provides sorting and cleanup commands for space contents.
// Uses command context payloads so actions target the selected space.
// Capability: commands, menu_contributions, settings_pages
class SpaceOrganizerPlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    PluginContext m_context{};
    mutable std::chrono::steady_clock::time_point m_lastRefreshAt{};

    void RegisterSettings() const;
    void RegisterMenus() const;
    void RegisterCommands() const;

    void HandleOrganizeByType(const CommandContext& command) const;
    void HandleFlatten(const CommandContext& command) const;
    void HandleCleanupEmpty(const CommandContext& command) const;
    void HandleArchiveOld(const CommandContext& command) const;
    void HandleMoveLarge(const CommandContext& command) const;

    SpaceMetadata ResolveSpace(const CommandContext& command) const;
    bool GetBool(const std::wstring& key, const std::wstring& fallback) const;
    int GetInt(const std::wstring& key, int fallback) const;
    std::wstring GetText(const std::wstring& key, const std::wstring& fallback) const;
    bool IsPluginEnabled() const;
    bool ShouldLogActions() const;
    bool IsSafeModeEnabled() const;
    std::wstring GetDefaultMode() const;
    int GetRefreshIntervalSeconds() const;
    void Notify(const std::wstring& message) const;
    void RefreshSpaceWithThrottle(const std::wstring& spaceId) const;
    void LogInfo(const std::wstring& message) const;
    void LogWarn(const std::wstring& message) const;

    static std::wstring SanitizeExtension(const std::filesystem::path& path);
};
