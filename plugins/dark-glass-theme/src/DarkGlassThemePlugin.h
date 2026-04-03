#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>

// Community plugin: Dark Glass Theme
// Registers appearance settings for a translucent dark fence style.
// Capability: appearance, settings_pages
class DarkGlassThemePlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    PluginContext m_context{};
    mutable std::chrono::steady_clock::time_point m_lastApplyAt{};

    void RegisterMenus() const;
    void RegisterCommands() const;
    void HandleReapplyTheme(const CommandContext& command) const;
    void HandlePreviewToggle(const CommandContext& command) const;

    bool GetBool(const std::wstring& key, bool fallback) const;
    int GetInt(const std::wstring& key, int fallback) const;
    void Notify(const std::wstring& message) const;
    void ApplyThemeToAllFences(bool bypassThrottle) const;
    void RefreshAllFencesWithThrottle() const;
};
