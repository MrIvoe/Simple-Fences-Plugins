#include "NetworkDriveSpacePlugin.h"

#include "core/Diagnostics.h"
#include "extensions/SpaceExtensionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

PluginManifest NetworkDriveSpacePlugin::GetManifest() const
{
    PluginManifest m;
    m.id           = L"community.network_drive_space";
    m.displayName  = L"Network Drive Space";
    m.version = L"1.2.2";
    m.description  = L"Space content provider for UNC paths and mapped network drives.";
    m.minHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.capabilities = {L"space_content_provider", L"settings_pages"};
    return m;
}

bool NetworkDriveSpacePlugin::Initialize(const PluginContext& context)
{
    m_context = context;

    // Register the content provider type so spaces can use it.
    if (context.spaceExtensionRegistry)
    {
        SpaceContentProviderDescriptor desc;
        desc.providerId    = L"community.network_drive_space";
        desc.contentType   = L"network_drive";
        desc.displayName   = L"Network Drive";
        desc.isCoreDefault = false;
        context.spaceExtensionRegistry->RegisterContentProvider(desc);
    }

    // Settings page
    if (context.settingsRegistry)
    {
        PluginSettingsPage page;
        page.pluginId = L"community.network_drive_space";
        page.pageId   = L"net_space.general";
        page.title    = L"Network Drive";
        page.order    = 10;

        PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 60, false);

        page.fields.push_back(SettingsFieldDescriptor{
            L"net_space.general.default_path",
            L"Default UNC path",
            L"Path pre-filled when creating a new network drive space (e.g. \\\\server\\share).",
            SettingsFieldType::String, L"", {}, 10
        });

        page.fields.push_back(SettingsFieldDescriptor{
            L"net_space.general.reconnect_on_load",
            L"Reconnect on startup",
            L"Attempt to re-connect disconnected network drives when SimpleSpaces launches.",
            SettingsFieldType::Bool, L"true", {}, 20
        });

        page.fields.push_back(SettingsFieldDescriptor{
            L"net_space.general.offline_label",
            L"Offline mode",
            L"How to display a space when the network path is unreachable.",
            SettingsFieldType::Enum, L"grayed",
            {
                {L"grayed",  L"Show greyed-out icon"},
                {L"hidden",  L"Hide space"},
                {L"error",   L"Show error message"},
            },
            30
        });

        page.fields.push_back(SettingsFieldDescriptor{
            L"net_space.general.refresh_seconds",
            L"Refresh interval (seconds)",
            L"How often the plugin should re-check the network path status.",
            SettingsFieldType::Int, L"30", {}, 40
        });

        page.fields.push_back(SettingsFieldDescriptor{
            L"net_space.general.show_connection_status",
            L"Show connection badge",
            L"Display a status indicator when the path is online, reconnecting, or offline.",
            SettingsFieldType::Bool, L"true", {}, 50
        });

        page.fields.push_back(SettingsFieldDescriptor{
            L"net_space.general.prefer_drive_label",
            L"Prefer mapped drive label",
            L"Use the mapped drive letter label when one is available.",
            SettingsFieldType::Bool, L"true", {}, 60
        });

        page.fields.push_back(SettingsFieldDescriptor{
            L"net_space.general.auto_open_on_reconnect",
            L"Auto-open after reconnect",
            L"Open the space automatically when the network location becomes available again.",
            SettingsFieldType::Bool, L"false", {}, 70
        });

        page.fields.push_back(SettingsFieldDescriptor{
            L"net_space.general.display_alias",
            L"Display alias",
            L"Optional friendly label shown instead of the raw UNC path.",
            SettingsFieldType::String, L"", {}, 80
        });

        page.fields.push_back(SettingsFieldDescriptor{
            L"net_space.general.open_action",
            L"Default open action",
            L"Choose what happens when the user opens the space title or header action.",
            SettingsFieldType::Enum, L"browse_root",
            {
                {L"browse_root", L"Browse root path"},
                {L"open_in_explorer", L"Open in Explorer"},
                {L"show_status", L"Show status/details"},
            },
            90
        });

        context.settingsRegistry->RegisterPage(std::move(page));

        PluginSettingsPage behaviorPage;
        behaviorPage.pluginId = L"community.network_drive_space";
        behaviorPage.pageId   = L"net_space.behavior";
        behaviorPage.title    = L"Availability";
        behaviorPage.order    = 20;

        behaviorPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.behavior.offline_cache_mode",
            L"Offline cache mode",
            L"Choose how aggressively the provider should rely on cached metadata when offline.",
            SettingsFieldType::Enum, L"metadata_only",
            {
                {L"off", L"Do not cache"},
                {L"metadata_only", L"Cache metadata only"},
                {L"metadata_and_icons", L"Cache metadata and icons"},
            },
            10
        });

        behaviorPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.behavior.warn_on_slow_share",
            L"Warn on slow share",
            L"Show a warning when the network path responds slowly.",
            SettingsFieldType::Bool, L"true", {}, 20
        });

        behaviorPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.behavior.retry_count",
            L"Reconnect retries",
            L"Maximum number of reconnect attempts before marking the space offline.",
            SettingsFieldType::Int, L"3", {}, 30
        });

        behaviorPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.behavior.connect_timeout_seconds",
            L"Connection timeout (seconds)",
            L"Maximum time to wait for a network-path health check before treating it as unavailable.",
            SettingsFieldType::Int, L"10", {}, 40
        });

        behaviorPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.behavior.show_last_seen_time",
            L"Show last-seen timestamp",
            L"Show when the provider last successfully reached the network location.",
            SettingsFieldType::Bool, L"true", {}, 50
        });

        behaviorPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.behavior.background_refresh_on_metered",
            L"Refresh on metered networks",
            L"Allow background refresh checks even when Windows marks the connection as metered.",
            SettingsFieldType::Bool, L"false", {}, 60
        });

        context.settingsRegistry->RegisterPage(std::move(behaviorPage));

        PluginSettingsPage accessPage;
        accessPage.pluginId = L"community.network_drive_space";
        accessPage.pageId   = L"net_space.access";
        accessPage.title    = L"Access";
        accessPage.order    = 30;

        accessPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.access.auth_mode",
            L"Authentication mode",
            L"Choose how the host should handle credentials for the network location.",
            SettingsFieldType::Enum, L"windows_session",
            {
                {L"windows_session", L"Use current Windows session"},
                {L"prompt", L"Prompt when needed"},
                {L"saved_profile", L"Use saved credential profile"},
            },
            10
        });

        accessPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.access.remember_successful_credentials",
            L"Remember successful credentials",
            L"Allow the host to remember a successful credential choice for later reconnects.",
            SettingsFieldType::Bool, L"true", {}, 20
        });

        accessPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.access.read_only_preference",
            L"Prefer read-only access",
            L"Prefer a read-only browsing flow when the host supports multiple access modes.",
            SettingsFieldType::Bool, L"false", {}, 30
        });

        context.settingsRegistry->RegisterPage(std::move(accessPage));

        PluginSettingsPage filterPage;
        filterPage.pluginId = L"community.network_drive_space";
        filterPage.pageId   = L"net_space.filter";
        filterPage.title    = L"Filter";
        filterPage.order    = 40;

        filterPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.filter.hide_hidden",
            L"Hide hidden items",
            L"Exclude items with the hidden attribute from the space listing.",
            SettingsFieldType::Bool, L"true", {}, 10
        });

        filterPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.filter.hide_system",
            L"Hide system items",
            L"Exclude items with the system attribute from the space listing.",
            SettingsFieldType::Bool, L"true", {}, 20
        });

        filterPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.filter.extensions_allowlist",
            L"Extension allowlist",
            L"Comma-separated list of file extensions to show (e.g. docx,xlsx,pdf). Leave blank to allow all.",
            SettingsFieldType::String, L"", {}, 30
        });

        filterPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.filter.extensions_blocklist",
            L"Extension blocklist",
            L"Comma-separated list of file extensions to hide (e.g. tmp,bak). Evaluated after the allowlist.",
            SettingsFieldType::String, L"", {}, 40
        });

        filterPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.filter.max_items",
            L"Max listed items",
            L"Cap the number of items shown per space for large shares. 0 = unlimited.",
            SettingsFieldType::Int, L"500", {}, 50
        });

        filterPage.fields.push_back(SettingsFieldDescriptor{
            L"net_space.filter.name_pattern",
            L"Name filter pattern",
            L"Optional wildcard pattern to restrict shown items by name (e.g. Report*). Leave blank to show all.",
            SettingsFieldType::String, L"", {}, 60
        });

        context.settingsRegistry->RegisterPage(std::move(filterPage));
    }

    RefreshNetworkSpacesWithThrottle();
    Notify(L"Network Drive provider initialized.");

    return true;
}

void NetworkDriveSpacePlugin::Shutdown()
{
    Notify(L"Network Drive provider shutdown.");
}

bool NetworkDriveSpacePlugin::GetBool(const std::wstring& key, bool fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    return m_context.settingsRegistry->GetValue(key, fallback ? L"true" : L"false") == L"true";
}

int NetworkDriveSpacePlugin::GetInt(const std::wstring& key, int fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    try
    {
        return std::stoi(m_context.settingsRegistry->GetValue(key, std::to_wstring(fallback)));
    }
    catch (...)
    {
        return fallback;
    }
}

void NetworkDriveSpacePlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[NetworkDriveSpace][Notification] " + message);
}

void NetworkDriveSpacePlugin::RefreshNetworkSpacesWithThrottle() const
{
    if (!m_context.appCommands)
    {
        return;
    }

    int seconds = GetInt(L"plugin.refresh_interval_seconds", 60);
    if (seconds < 1)
    {
        seconds = 1;
    }

    const auto now = std::chrono::steady_clock::now();
    if (m_lastSyncAt.time_since_epoch().count() != 0 && (now - m_lastSyncAt) < std::chrono::seconds(seconds))
    {
        return;
    }

    m_lastSyncAt = now;
    const auto ids = m_context.appCommands->GetAllSpaceIds();
    for (const auto& id : ids)
    {
        const SpaceMetadata space = m_context.appCommands->GetSpaceMetadata(id);
        if (space.contentType == L"network_drive")
        {
            m_context.appCommands->RefreshSpace(id);
        }
    }
}



