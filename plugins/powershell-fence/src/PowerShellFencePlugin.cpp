#include "PowerShellFencePlugin.h"

#include "extensions/FenceExtensionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

PluginManifest PowerShellFencePlugin::GetManifest() const
{
    PluginManifest m;
    m.id               = L"community.powershell_fence";
    m.displayName      = L"PowerShell Workspace Fence";
    m.version = L"0.2.1";
    m.description      = L"Prototype PowerShell workspace fence with persisted startup, admin, view, and safety controls.";
    m.enabledByDefault = false;
    m.capabilities     = {L"fence_content_provider", L"settings_pages"};
    return m;
}

bool PowerShellFencePlugin::Initialize(const PluginContext& context)
{
    m_context = context;

    if (context.fenceExtensionRegistry)
    {
        FenceContentProviderDescriptor descriptor;
        descriptor.providerId    = L"community.powershell_fence";
        descriptor.contentType   = L"powershell_workspace";
        descriptor.displayName   = L"PowerShell Workspace";
        descriptor.isCoreDefault = false;
        context.fenceExtensionRegistry->RegisterContentProvider(descriptor);
    }

    if (!context.settingsRegistry)
    {
        return true;
    }

    PluginSettingsPage startupPage;
    startupPage.pluginId = L"community.powershell_fence";
    startupPage.pageId   = L"ps_fence.startup";
    startupPage.title    = L"Startup";
    startupPage.order    = 10;

    PluginUiPatterns::AppendBaselineSettingsFields(startupPage.fields, 1, 60, false);

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.startup.mode",
        L"Startup mode",
        L"Choose what the fence loads when it is opened.",
        SettingsFieldType::Enum, L"blank",
        {
            {L"blank", L"Blank session"},
            {L"profile", L"Load PowerShell profile"},
            {L"script", L"Run startup script"},
        },
        10
    });

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.startup.script_path",
        L"Startup script",
        L"Optional script path to launch when startup mode is set to Run startup script.",
        SettingsFieldType::String, L"", {}, 20
    });

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.startup.working_directory",
        L"Working directory",
        L"Initial working directory for the PowerShell workspace.",
        SettingsFieldType::String, L"", {}, 30
    });

    startupPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.startup.execution_policy",
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
        L"ps_fence.startup.shell_variant",
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
        L"ps_fence.startup.import_profile_modules",
        L"Import profile modules",
        L"Allow the host to load modules referenced by the selected startup profile or script.",
        SettingsFieldType::Bool, L"true", {}, 60
    });

    context.settingsRegistry->RegisterPage(std::move(startupPage));

    PluginSettingsPage adminPage;
    adminPage.pluginId = L"community.powershell_fence";
    adminPage.pageId   = L"ps_fence.admin";
    adminPage.title    = L"Admin";
    adminPage.order    = 15;

    adminPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.admin.mode",
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
        L"ps_fence.admin.startup_elevated",
        L"Start elevated on open",
        L"Launch the first session for the fence in admin mode when allowed by the selected admin mode.",
        SettingsFieldType::Bool, L"false", {}, 20
    });

    adminPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.admin.separate_profile",
        L"Use separate admin startup profile",
        L"Allow an elevated workspace to use a different script or profile path than standard sessions.",
        SettingsFieldType::Bool, L"true", {}, 30
    });

    adminPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.admin.profile_path",
        L"Admin startup script",
        L"Optional elevated-session startup script or profile path.",
        SettingsFieldType::String, L"", {}, 40
    });

    adminPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.admin.badge_visibility",
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
    viewPage.pluginId = L"community.powershell_fence";
    viewPage.pageId   = L"ps_fence.view";
    viewPage.title    = L"View";
    viewPage.order    = 20;

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.view.launch_on_open",
        L"Launch session when opened",
        L"Start or attach to a PowerShell session when the fence opens.",
        SettingsFieldType::Bool, L"true", {}, 10
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.view.reuse_session",
        L"Reuse existing session",
        L"Reconnect to an existing session for the same fence instead of launching a new one.",
        SettingsFieldType::Bool, L"true", {}, 20
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.view.show_toolbar",
        L"Show command toolbar",
        L"Show quick actions such as New Session, Clear, and Open Folder.",
        SettingsFieldType::Bool, L"true", {}, 30
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.view.focus_behavior",
        L"Focus behavior",
        L"Choose where focus goes when the fence is activated.",
        SettingsFieldType::Enum, L"terminal",
        {
            {L"retain", L"Retain current focus"},
            {L"terminal", L"Focus terminal"},
            {L"last_active", L"Restore last active control"},
        },
        40
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.view.show_exit_code",
        L"Show last exit code",
        L"Show the most recent command or script exit code in the workspace UI.",
        SettingsFieldType::Bool, L"true", {}, 50
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.view.show_breadcrumbs",
        L"Show path breadcrumbs",
        L"Show the current working directory as breadcrumbs above the session surface.",
        SettingsFieldType::Bool, L"true", {}, 60
    });

    viewPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.view.max_scrollback_lines",
        L"Scrollback limit",
        L"Maximum number of lines the host should keep in memory for the session history.",
        SettingsFieldType::Int, L"5000", {}, 70
    });

    context.settingsRegistry->RegisterPage(std::move(viewPage));

    PluginSettingsPage safetyPage;
    safetyPage.pluginId = L"community.powershell_fence";
    safetyPage.pageId   = L"ps_fence.safety";
    safetyPage.title    = L"Safety";
    safetyPage.order    = 30;

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.safety.allow_user_scripts",
        L"Allow user script launch",
        L"Allow manually selected scripts to run from inside the fence workspace.",
        SettingsFieldType::Bool, L"false", {}, 10
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.safety.restrict_to_working_directory",
        L"Restrict to working directory",
        L"Limit startup and quick actions to the configured working directory.",
        SettingsFieldType::Bool, L"true", {}, 20
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.safety.confirm_admin_actions",
        L"Confirm elevated actions",
        L"Require confirmation before any host flow launches an elevated PowerShell action.",
        SettingsFieldType::Bool, L"true", {}, 30
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.safety.block_network_scripts",
        L"Block network script paths",
        L"Prevent startup or quick-run actions from launching scripts directly from UNC or mapped-network paths.",
        SettingsFieldType::Bool, L"true", {}, 40
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.safety.require_signed_scripts",
        L"Require signed scripts",
        L"Only allow script launch flows when the target script is signed and trusted.",
        SettingsFieldType::Bool, L"false", {}, 50
    });

    safetyPage.fields.push_back(SettingsFieldDescriptor{
        L"ps_fence.safety.clear_session_on_close",
        L"Clear transient session state on close",
        L"Clear temporary command history and volatile session output when the fence closes.",
        SettingsFieldType::Bool, L"false", {}, 60
    });

    context.settingsRegistry->RegisterPage(std::move(safetyPage));

    RefreshWorkspaceFencesWithThrottle();
    Notify(L"PowerShell workspace provider initialized.");
    return true;
}

void PowerShellFencePlugin::Shutdown()
{
    Notify(L"PowerShell workspace provider shutdown.");
}

bool PowerShellFencePlugin::GetBool(const std::wstring& key, bool fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    return m_context.settingsRegistry->GetValue(key, fallback ? L"true" : L"false") == L"true";
}

int PowerShellFencePlugin::GetInt(const std::wstring& key, int fallback) const
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

void PowerShellFencePlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[PowerShellFence][Notification] " + message);
}

void PowerShellFencePlugin::RefreshWorkspaceFencesWithThrottle() const
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
    const auto ids = m_context.appCommands->GetAllFenceIds();
    for (const auto& id : ids)
    {
        const FenceMetadata fence = m_context.appCommands->GetFenceMetadata(id);
        if (fence.contentType == L"powershell_workspace")
        {
            m_context.appCommands->RefreshFence(id);
        }
    }
}
