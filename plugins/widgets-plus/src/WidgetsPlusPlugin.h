#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>
#include <unordered_map>

class WidgetsPlusPlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    PluginContext m_context{};
    mutable bool m_paused = false;
    mutable std::unordered_map<std::wstring, std::chrono::steady_clock::time_point> m_lastRefreshAtByFence;

    void RegisterSettings() const;
    void RegisterMenus() const;
    void RegisterCommands() const;

    void HandleAddClock(const CommandContext& command) const;
    void HandleAddNotes(const CommandContext& command) const;
    void HandleAddChecklist(const CommandContext& command) const;
    void HandleRefreshAll(const CommandContext& command) const;
    void HandlePauseToggle(const CommandContext& command) const;

    bool GetBool(const std::wstring& key, bool fallback) const;
    int GetInt(const std::wstring& key, int fallback) const;
    std::wstring GetString(const std::wstring& key, const std::wstring& fallback) const;
    void Notify(const std::wstring& message) const;
    void RefreshFenceWithThrottle(const std::wstring& fenceId) const;
    void LogInfo(const std::wstring& message) const;
};
