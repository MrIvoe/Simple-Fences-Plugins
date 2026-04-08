#include "PowerShellSpacePlugin.h"

#include "core/Diagnostics.h"
#include "extensions/SpaceExtensionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

PluginManifest PowerShellSpacePlugin::GetManifest() const
{
    PluginManifest m;
    m.id               = L"community.powershell_space";
    m.displayName      = L"PowerShell Workspace Space";
    m.version = L"0.2.2";
    m.description      = L"Prototype PowerShell workspace space with persisted startup, admin, view, and safety controls.";
    m.minHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.enabledByDefault = false;
    m.capabilities     = {L"space_content_provider", L"settings_pages"};
    return m;
}

bool PowerShellSpacePlugin::Initialize(const PluginContext& context)
{
    m_context = context;

    if (context.spaceExtensionRegistry)
    {
        SpaceContentProviderDescriptor descriptor;
        descriptor.providerId    = L"community.powershell_space";
        descriptor.contentType   = L"powershell_workspace";
        descriptor.displayName   = L"PowerShell Workspace";
        descriptor.isCoreDefault = false;
        context.spaceExtensionRegistry->RegisterContentProvider(descriptor);
    }

    if (!context.settingsRegistry)
    {
        return true;
    }

    PluginSettingsPage startupPage;
    startupPage.pluginId = L"community.powershell_space";
    startupPage.pageId   = L"ps_space.startup";
    startupPage.title    = L"Startup";
    startupPage.order    = 10;

    PluginUiPatterns::AppendBaselineSettingsFields(startupPage.fields, 1, 60, false);

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.startup.mode",
        L"Startup mode",
        L"Choose what the space loads when it is opened.",
        SettingsFieldType::Enum, L"blank",
        {
            {L"blank", L"Blank session"},
            {L"profile", L"Load PowerShell profile"},
            {L"script", L"Run startup script"},
        },
        10
    });

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.startup.script_path",
        L"Startup script",
        L"Optional script path to launch when startup mode is set to Run startup script.",
        SettingsFieldType::String, L"", {}, 20
    });

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.startup.working_directory",
        L"Working directory",
        L"Initial working directory for the PowerShell workspace.",
        SettingsFieldType::String, L"", {}, 30
    });

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.startup.execution_policy",
        L"Execution policy",
        L"Execution policy hint the host can apply when launching the workspace.",
        SettingsFieldType::Enum, L"default",
        {
            {L"default", L"Use system default"},
            {L"bypass", L"Bypass"},
            {L"remotesigned", L"RemoteSigned"},
            {L"allsigned", L"AllSigned"},
        },
        40
    });

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.startup.shell_variant",
        L"Shell variant",
        L"Choose which PowerShell host the workspace should prefer.",
        SettingsFieldType::Enum, L"pwsh",
        {
            {L"pwsh", L"PowerShell 7+ (pwsh)"},
            {L"windows_powershell", L"Windows PowerShell"},
            {L"auto", L"Auto-detect"},
        },
        50
    });

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.startup.import_profile_modules",
        L"Import profile modules",
        L"Allow the host to load modules referenced by the selected startup profile or script.",
        SettingsFieldType::Bool, L"true", {}, 60
    });

    context.settingsRegistry->RegisterPage(std::move(startupPage));

    PluginSettingsPage adminPage;
    adminPage.pluginId = L"community.powershell_space";
    adminPage.pageId   = L"ps_space.admin";
    adminPage.title    = L"Admin";
    adminPage.order    = 15;

    adminPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.admin.mode",
        L"Admin mode",
        L"Choose when the workspace should request or use elevated PowerShell sessions.",
        SettingsFieldType::Enum, L"ask",
        {
            {L"never", L"Never elevate"},
            {L"ask", L"Ask when needed"},
            {L"always", L"Always launch elevated"},
        },
        10
    });

    adminPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.admin.startup_elevated",
        L"Start elevated on open",
        L"Launch the first session for the space in admin mode when allowed by the selected admin mode.",
        SettingsFieldType::Bool, L"false", {}, 20
    });

    adminPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.admin.separate_profile",
        L"Use separate admin startup profile",
        L"Allow an elevated workspace to use a different script or profile path than standard sessions.",
        SettingsFieldType::Bool, L"true", {}, 30
    });

    adminPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.admin.profile_path",
        L"Admin startup script",
        L"Optional elevated-session startup script or profile path.",
        SettingsFieldType::String, L"", {}, 40
    });

    adminPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.admin.badge_visibility",
        L"Admin badge",
        L"Choose how the workspace indicates that it is running elevated.",
        SettingsFieldType::Enum, L"icon_and_text",
        {
            {L"hidden", L"Hidden"},
            {L"icon_only", L"Icon only"},
            {L"icon_and_text", L"Icon and text"},
        },
        50
    });

    context.settingsRegistry->RegisterPage(std::move(adminPage));

    PluginSettingsPage viewPage;
    viewPage.pluginId = L"community.powershell_space";
    viewPage.pageId   = L"ps_space.view";
    viewPage.title    = L"View";
    viewPage.order    = 20;

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.view.launch_on_open",
        L"Launch session when opened",
        L"Start or attach to a PowerShell session when the space opens.",
        SettingsFieldType::Bool, L"true", {}, 10
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.view.reuse_session",
        L"Reuse existing session",
        L"Reconnect to an existing session for the same space instead of launching a new one.",
        SettingsFieldType::Bool, L"true", {}, 20
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.view.show_toolbar",
        L"Show command toolbar",
        L"Show quick actions such as New Session, Clear, and Open Folder.",
        SettingsFieldType::Bool, L"true", {}, 30
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.view.focus_behavior",
        L"Focus behavior",
        L"Choose where focus goes when the space is activated.",
        SettingsFieldType::Enum, L"terminal",
        {
            {L"retain", L"Retain current focus"},
            {L"terminal", L"Focus terminal"},
            {L"last_active", L"Restore last active control"},
        },
        40
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.view.show_exit_code",
        L"Show last exit code",
        L"Show the most recent command or script exit code in the workspace UI.",
        SettingsFieldType::Bool, L"true", {}, 50
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.view.show_breadcrumbs",
        L"Show path breadcrumbs",
        L"Show the current working directory as breadcrumbs above the session surface.",
        SettingsFieldType::Bool, L"true", {}, 60
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.view.max_scrollback_lines",
        L"Scrollback limit",
        L"Maximum number of lines the host should keep in memory for the session history.",
        SettingsFieldType::Int, L"5000", {}, 70
    });

    context.settingsRegistry->RegisterPage(std::move(viewPage));

    PluginSettingsPage safetyPage;
    safetyPage.pluginId = L"community.powershell_space";
    safetyPage.pageId   = L"ps_space.safety";
    safetyPage.title    = L"Safety";
    safetyPage.order    = 30;

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.safety.allow_user_scripts",
        L"Allow user script launch",
        L"Allow manually selected scripts to run from inside the space workspace.",
        SettingsFieldType::Bool, L"false", {}, 10
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.safety.restrict_to_working_directory",
        L"Restrict to working directory",
        L"Limit startup and quick actions to the configured working directory.",
        SettingsFieldType::Bool, L"true", {}, 20
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.safety.confirm_admin_actions",
        L"Confirm elevated actions",
        L"Require confirmation before any host flow launches an elevated PowerShell action.",
        SettingsFieldType::Bool, L"true", {}, 30
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.safety.block_network_scripts",
        L"Block network script paths",
        L"Prevent startup or quick-run actions from launching scripts directly from UNC or mapped-network paths.",
        SettingsFieldType::Bool, L"true", {}, 40
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.safety.require_signed_scripts",
        L"Require signed scripts",
        L"Only allow script launch flows when the target script is signed and trusted.",
        SettingsFieldType::Bool, L"false", {}, 50
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.safety.clear_session_on_close",
        L"Clear transient session state on close",
        L"Clear temporary command history and volatile session output when the space closes.",
        SettingsFieldType::Bool, L"false", {}, 60
    });

    context.settingsRegistry->RegisterPage(std::move(safetyPage));

    PluginSettingsPage outputPage;
    outputPage.pluginId = L"community.powershell_space";
    outputPage.pageId   = L"ps_space.output";
    outputPage.title    = L"Output";
    outputPage.order    = 40;

    outputPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.output.color_scheme",
        L"Output color scheme",
        L"Color palette applied to terminal output inside the workspace.",
        SettingsFieldType::Enum, L"auto",
        {
            {L"auto",        L"Auto (follow system)"},
            {L"dark",        L"Dark"},
            {L"light",       L"Light"},
            {L"high_contrast", L"High contrast"},
        },
        10
    });

    outputPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.output.wrap_long_lines",
        L"Wrap long lines",
        L"Soft-wrap output lines that exceed the terminal width rather than scrolling horizontally.",
        SettingsFieldType::Bool, L"true", {}, 20
    });

    outputPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.output.font_size_pt",
        L"Font size (pt)",
        L"Terminal font size in points. Set to 0 to inherit the system default.",
        SettingsFieldType::Int, L"0", {}, 30
    });

    outputPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.output.show_timestamp_prefix",
        L"Show timestamp prefix",
        L"Prefix each output line with a timestamp when the host renderer supports it.",
        SettingsFieldType::Bool, L"false", {}, 40
    });

    outputPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.output.history_persist",
        L"Persist command history",
        L"Save and restore command history between space sessions.",
        SettingsFieldType::Bool, L"true", {}, 50
    });

    outputPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.output.history_max_entries",
        L"Max history entries",
        L"Maximum number of past commands retained in the persistent history store.",
        SettingsFieldType::Int, L"500", {}, 60
    });

    outputPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_space.output.copy_on_select",
        L"Copy on select",
        L"Automatically copy selected output text to the clipboard.",
        SettingsFieldType::Bool, L"false", {}, 70
    });

    context.settingsRegistry->RegisterPage(std::move(outputPage));

    RefreshWorkspaceSpacesWithThrottle();
    Notify(L"PowerShell workspace provider initialized.");
    return true;
}

void PowerShellSpacePlugin::Shutdown()
{
    Notify(L"PowerShell workspace provider shutdown.");
}

bool PowerShellSpacePlugin::GetBool(const std::wstring& key, bool fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    return m_context.settingsRegistry->GetValue(key, fallback ? L"true" : L"false") == L"true";
}

int PowerShellSpacePlugin::GetInt(const std::wstring& key, int fallback) const
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

void PowerShellSpacePlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[PowerShellSpace][Notification] " + message);
}

void PowerShellSpacePlugin::RefreshWorkspaceSpacesWithThrottle() const
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
        if (space.contentType == L"powershell_workspace")
        {
            m_context.appCommands->RefreshSpace(id);
        }
    }
}

