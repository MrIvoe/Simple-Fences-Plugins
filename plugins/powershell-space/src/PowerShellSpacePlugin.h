#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>

// Community plugin: PowerShell Workspace Space (Prototype)
// Registers a space_content_provider and settings pages for a PowerShell-driven workspace.
// Capability: space_content_provider, settings_pages
class PowerShellSpacePlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    PluginContext m_context{};
    mutable std::chrono::steady_clock::time_point m_lastSyncAt{};

    bool GetBool(const std::wstring& key, bool fallback) const;
    int GetInt(const std::wstring& key, int fallback) const;
    void Notify(const std::wstring& message) const;
    void RefreshWorkspaceSpacesWithThrottle() const;
};
