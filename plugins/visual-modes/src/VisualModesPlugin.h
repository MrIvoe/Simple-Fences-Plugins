#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>

class VisualModesPlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    PluginContext m_context{};
    mutable std::chrono::steady_clock::time_point m_lastApplyAt{};

    void RegisterSettings() const;
    void RegisterMenus() const;
    void RegisterCommands() const;

    void HandleThemeSwitch(const CommandContext& command) const;
    void HandleCompactToggle(const CommandContext& command) const;
    void HandlePresentationToggle(const CommandContext& command) const;
    void HandleApplyCurrentToSpace(const CommandContext& command) const;
    void HandleResetSpace(const CommandContext& command) const;

    std::wstring GetSetting(const std::wstring& key, const std::wstring& fallback) const;
    bool GetSettingBool(const std::wstring& key, bool fallback) const;
    bool IsPluginEnabled() const;
    bool ShouldLogActions() const;
    bool IsSafeModeEnabled() const;
    int GetRefreshIntervalSeconds() const;
    void Notify(const std::wstring& message) const;
    bool CanApplyNow() const;
    void SetPreset(const std::wstring& preset) const;
    std::wstring ResolveTargetSpaceId(const CommandContext& command) const;
    SpacePresentationSettings BuildPresentationFromPreset(const std::wstring& preset) const;
    void ApplyPresetToSpace(const std::wstring& spaceId, const std::wstring& preset, bool applyToAll) const;
    void LogInfo(const std::wstring& message) const;
};
