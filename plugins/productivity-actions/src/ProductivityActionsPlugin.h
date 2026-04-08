#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>
#include <filesystem>

class ProductivityActionsPlugin final : public IPlugin
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

    void HandleCreateProjectSpace(const CommandContext& command) const;
    void HandleArchiveOld(const CommandContext& command) const;
    void HandleOpenAll(const CommandContext& command) const;
    void HandleBatchRename(const CommandContext& command) const;
    void HandleSnapshotSave(const CommandContext& command) const;

    std::wstring ResolveSpaceId(const CommandContext& command) const;
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
    void LogInfo(const std::wstring& message) const;
    void LogWarn(const std::wstring& message) const;

    static std::filesystem::path BuildUniquePath(const std::filesystem::path& target);
};
