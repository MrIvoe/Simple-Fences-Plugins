#pragma once

#include "extensions/SettingsSchema.h"

#include <string>
#include <vector>

namespace PluginUiPatterns
{
    inline std::wstring BoolDefault(bool value)
    {
        return value ? L"true" : L"false";
    }

    inline void AppendBaselineSettingsFields(std::vector<SettingsFieldDescriptor>& fields,
                                             int firstOrder,
                                             int refreshIntervalSeconds,
                                             bool showNotificationsDefault)
    {
        if (refreshIntervalSeconds < 1)
        {
            refreshIntervalSeconds = 1;
        }

        fields.push_back(SettingsFieldDescriptor{
            L"plugin.show_notifications",
            L"Show notifications",
            L"Emit user-facing notification events to diagnostics.",
            SettingsFieldType::Bool,
            BoolDefault(showNotificationsDefault),
            {},
            firstOrder});

        fields.push_back(SettingsFieldDescriptor{
            L"plugin.refresh_interval_seconds",
            L"Refresh interval (s)",
            L"Minimum interval between host refresh operations triggered by this plugin.",
            SettingsFieldType::Int,
            std::to_wstring(refreshIntervalSeconds),
            {},
            firstOrder + 1});
    }
}
