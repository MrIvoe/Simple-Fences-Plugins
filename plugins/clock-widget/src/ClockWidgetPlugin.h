#pragma once

#include "extensions/PluginContracts.h"

#include <chrono>

// Community plugin: Clock Widget
// Renders a live clock inside a space widget panel.
class ClockWidgetPlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;

private:
    PluginContext m_context{};
    mutable std::chrono::steady_clock::time_point m_lastRefreshAt{};

    bool GetBool(const std::wstring& key, bool fallback) const;
    int GetInt(const std::wstring& key, int fallback) const;
    void Notify(const std::wstring& message) const;
    void RefreshWidgetPanelsWithThrottle() const;
};
