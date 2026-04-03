#include "ProductivityActionsPlugin.h"

#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

#include <chrono>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <shellapi.h>
#endif

namespace fs = std::filesystem;

PluginManifest ProductivityActionsPlugin::GetManifest() const
{
    PluginManifest m;
    m.id = L"community.productivity_actions";
    m.displayName = L"Productivity Actions";
    m.version = L"1.1.1";
    m.description = L"Adds practical multi-step commands for daily fence workflows.";
    m.minHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.capabilities = {L"commands", L"tray_contributions", L"settings_pages", L"desktop_context"};
    return m;
}

bool ProductivityActionsPlugin::Initialize(const PluginContext& context)
{
    m_context = context;
    if (!m_context.commandDispatcher || !m_context.settingsRegistry || !m_context.appCommands)
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Error(L"ProductivityActionsPlugin: Missing required host services");
        }
        return false;
    }

    RegisterSettings();
    RegisterMenus();
    RegisterCommands();
    LogInfo(L"Initialized");
    return true;
}

void ProductivityActionsPlugin::Shutdown()
{
    LogInfo(L"Shutdown");
}

void ProductivityActionsPlugin::RegisterSettings() const
{
    PluginSettingsPage page;
    page.pluginId = L"community.productivity_actions";
    page.pageId = L"productivity.general";
    page.title = L"Productivity Actions";
    page.order = 60;

    page.fields.push_back(SettingsFieldDescriptor{L"plugin.enabled", L"Enable plugin", L"Master toggle for Productivity Actions behavior.", SettingsFieldType::Bool, L"true", {}, 1});
    page.fields.push_back(SettingsFieldDescriptor{L"plugin.log_actions", L"Log actions", L"Write productivity action operations to diagnostics.", SettingsFieldType::Bool, L"true", {}, 2});
    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 120, false);

    page.fields.push_back(SettingsFieldDescriptor{L"prod.enabled", L"Enable productivity actions", L"Master toggle for productivity actions.", SettingsFieldType::Bool, L"true", {}, 10});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.confirm_batch", L"Confirm batch operations", L"Require confirmation for bulk actions.", SettingsFieldType::Bool, L"true", {}, 20});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.templates.default", L"Default template", L"Default template when creating project fences.", SettingsFieldType::Enum, L"general", {{L"dev", L"Development"}, {L"design", L"Design"}, {L"media", L"Media"}, {L"general", L"General"}}, 30});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.archive.threshold_days", L"Archive threshold (days)", L"Files older than this are archived.", SettingsFieldType::Int, L"30", {}, 40});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.archive.action", L"Archive action", L"Move, copy, or prompt before archiving.", SettingsFieldType::Enum, L"move", {{L"move", L"Move"}, {L"copy", L"Copy"}, {L"prompt", L"Prompt"}}, 50});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.archive.destination", L"Archive destination", L"Optional destination folder. Empty uses fence-local _archive.", SettingsFieldType::String, L"", {}, 60});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.rename.pattern", L"Rename pattern", L"Pattern using {name} and {index} tokens.", SettingsFieldType::String, L"{name}_{index}", {}, 70});

    page.fields.push_back(SettingsFieldDescriptor{L"prod.open_all.max_count", L"Open All — max item count", L"Maximum number of items that Open All Items will open without confirmation. 0 = always confirm.", SettingsFieldType::Int, L"10", {}, 80});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.open_all.confirm_above", L"Open All — confirm above", L"Show a confirmation dialog before opening more than the configured max item count.", SettingsFieldType::Bool, L"true", {}, 90});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.snapshot.folder", L"Snapshot destination folder", L"Folder where fence snapshot files are saved. Leave blank to save alongside the fence source.", SettingsFieldType::String, L"", {}, 100});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.snapshot.include_metadata", L"Snapshot — include metadata", L"Include fence metadata such as provider type and display name in the snapshot file.", SettingsFieldType::Bool, L"true", {}, 110});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.snapshot.format", L"Snapshot format", L"File format for saved fence snapshots.", SettingsFieldType::Enum, L"json", {{L"json", L"JSON"}, {L"text", L"Plain text manifest"}}, 120});
    page.fields.push_back(SettingsFieldDescriptor{L"prod.project_fence.auto_create_subfolders", L"Auto-create project subfolders", L"Create standard subfolders (src, docs, assets) inside a new project fence.", SettingsFieldType::Bool, L"false", {}, 130});

    m_context.settingsRegistry->RegisterPage(std::move(page));
}

void ProductivityActionsPlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Create Project Fence", L"productivity.create_project_fence", 410, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Save Fence Snapshot", L"productivity.snapshot.save", 420, false});

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::FenceContext, L"Archive Old Items", L"productivity.archive_old", 610, true});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::FenceContext, L"Batch Rename Items", L"productivity.batch_rename", 620, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::FenceContext, L"Open All Items", L"productivity.open_all", 630, false});
}

void ProductivityActionsPlugin::RegisterCommands() const
{
    m_context.commandDispatcher->RegisterCommand(L"productivity.create_project_fence", [this](const CommandContext& command) { HandleCreateProjectFence(command); });
    m_context.commandDispatcher->RegisterCommand(L"productivity.archive_old", [this](const CommandContext& command) { HandleArchiveOld(command); });
    m_context.commandDispatcher->RegisterCommand(L"productivity.open_all", [this](const CommandContext& command) { HandleOpenAll(command); });
    m_context.commandDispatcher->RegisterCommand(L"productivity.batch_rename", [this](const CommandContext& command) { HandleBatchRename(command); });
    m_context.commandDispatcher->RegisterCommand(L"productivity.snapshot.save", [this](const CommandContext& command) { HandleSnapshotSave(command); });
}

void ProductivityActionsPlugin::HandleCreateProjectFence(const CommandContext&) const
{
    if (!IsPluginEnabled() || !GetBool(L"prod.enabled", true))
    {
        return;
    }

    const std::wstring preset = GetString(L"prod.templates.default", L"general");
    const std::wstring mode = GetDefaultMode();
    FenceCreateRequest request;
    request.contentType = L"file_collection";
    request.contentPluginId = L"core.file_collection";

    if (preset == L"dev")
    {
        request.title = L"Dev Project";
    }
    else if (preset == L"design")
    {
        request.title = L"Design Project";
    }
    else if (preset == L"media")
    {
        request.title = L"Media Project";
    }
    else
    {
        request.title = (mode == L"fast") ? L"Quick Project" : L"Project Fence";
    }

    const std::wstring created = m_context.appCommands->CreateFenceNearCursor(request);
    if (!created.empty())
    {
        LogInfo(L"Created project fence with id=" + created);
        Notify(L"Project fence created.");
    }
}

void ProductivityActionsPlugin::HandleArchiveOld(const CommandContext& command) const
{
    if (!IsPluginEnabled() || !GetBool(L"prod.enabled", true))
    {
        return;
    }

    const FenceMetadata fence = ResolveFence(command);
    if (fence.id.empty() || fence.backingFolderPath.empty())
    {
        LogWarn(L"archive_old skipped: no fence context");
        return;
    }

    const int days = GetInt(L"prod.archive.threshold_days", 30);
    std::wstring action = GetString(L"prod.archive.action", L"move");
    if (IsSafeModeEnabled())
    {
        action = L"prompt";
    }
    const std::wstring configuredDestination = GetString(L"prod.archive.destination", L"");
    fs::path destination = configuredDestination.empty()
        ? (fs::path(fence.backingFolderPath) / L"_archive")
        : fs::path(configuredDestination);

    const auto cutoff = fs::file_time_type::clock::now() - std::chrono::hours(24 * (days < 1 ? 1 : days));
    std::error_code ec;
    fs::create_directories(destination, ec);

    size_t changed = 0;
    for (const auto& entry : fs::directory_iterator(fence.backingFolderPath, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

        if (!entry.is_regular_file())
        {
            continue;
        }

        const auto writeTime = fs::last_write_time(entry.path(), ec);
        if (ec || writeTime >= cutoff)
        {
            ec.clear();
            continue;
        }

        const fs::path target = BuildUniquePath(destination / entry.path().filename());
        if (action == L"copy")
        {
            fs::copy(entry.path(), target, fs::copy_options::overwrite_existing, ec);
        }
        else if (action == L"prompt")
        {
            LogInfo(L"archive_old prompt mode: skipped move for " + entry.path().filename().wstring());
            continue;
        }
        else
        {
            fs::rename(entry.path(), target, ec);
            if (ec)
            {
                ec.clear();
                fs::copy(entry.path(), target, fs::copy_options::overwrite_existing, ec);
                if (!ec)
                {
                    fs::remove(entry.path(), ec);
                }
            }
        }

        if (!ec)
        {
            ++changed;
        }
        ec.clear();
    }

    RefreshFenceWithThrottle(fence.id);
    LogInfo(L"archive_old processed " + std::to_wstring(changed) + L" item(s)");
    Notify(L"Archive old items completed.");
}

void ProductivityActionsPlugin::HandleOpenAll(const CommandContext& command) const
{
    if (!IsPluginEnabled() || !GetBool(L"prod.enabled", true))
    {
        return;
    }

    const FenceMetadata fence = ResolveFence(command);
    if (fence.id.empty() || fence.backingFolderPath.empty())
    {
        LogWarn(L"open_all skipped: no fence context");
        return;
    }

#ifdef _WIN32
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(fence.backingFolderPath, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

        const std::wstring path = entry.path().wstring();
        ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
#else
    (void)command;
#endif

    LogInfo(L"open_all completed for fence " + fence.id);
    Notify(L"Open all items completed.");
}

void ProductivityActionsPlugin::HandleBatchRename(const CommandContext& command) const
{
    if (!IsPluginEnabled() || !GetBool(L"prod.enabled", true))
    {
        return;
    }

    const FenceMetadata fence = ResolveFence(command);
    if (fence.id.empty() || fence.backingFolderPath.empty())
    {
        LogWarn(L"batch_rename skipped: no fence context");
        return;
    }

    const std::wstring pattern = GetString(L"prod.rename.pattern", L"{name}_{index}");
    std::error_code ec;

    int index = 1;
    size_t renamed = 0;
    for (const auto& entry : fs::directory_iterator(fence.backingFolderPath, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::wstring originalStem = entry.path().stem().wstring();
        std::wstring nextName = pattern;
        size_t pos = std::wstring::npos;
        while ((pos = nextName.find(L"{name}")) != std::wstring::npos)
        {
            nextName.replace(pos, 6, originalStem);
        }
        while ((pos = nextName.find(L"{index}")) != std::wstring::npos)
        {
            nextName.replace(pos, 7, std::to_wstring(index));
        }

        const fs::path target = BuildUniquePath(entry.path().parent_path() / (nextName + entry.path().extension().wstring()));
        fs::rename(entry.path(), target, ec);
        if (!ec)
        {
            ++renamed;
        }
        ec.clear();
        ++index;
    }

    RefreshFenceWithThrottle(fence.id);
    LogInfo(L"batch_rename renamed " + std::to_wstring(renamed) + L" item(s)");
    Notify(L"Batch rename completed.");
}

void ProductivityActionsPlugin::HandleSnapshotSave(const CommandContext& command) const
{
    if (!IsPluginEnabled() || !GetBool(L"prod.enabled", true))
    {
        return;
    }

    const FenceMetadata fence = ResolveFence(command);
    if (fence.id.empty() || fence.backingFolderPath.empty())
    {
        LogWarn(L"snapshot.save skipped: no fence context");
        return;
    }

    const fs::path output = fs::path(fence.backingFolderPath) / L"fence-snapshot.txt";
    std::ofstream out(output, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        LogWarn(L"snapshot.save failed to open output file");
        return;
    }

    out << "Fence: ";
    out.write(reinterpret_cast<const char*>(fence.title.c_str()), static_cast<std::streamsize>(fence.title.size() * sizeof(wchar_t)));
    out << "\n";

    std::error_code ec;
    int count = 0;
    for (const auto& entry : fs::directory_iterator(fence.backingFolderPath, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

        const std::wstring name = entry.path().filename().wstring();
        out.write(reinterpret_cast<const char*>(name.c_str()), static_cast<std::streamsize>(name.size() * sizeof(wchar_t)));
        out << "\n";
        ++count;
    }

    out.close();
    LogInfo(L"snapshot.save captured " + std::to_wstring(count) + L" item(s)");
    Notify(L"Fence snapshot saved.");
}

std::wstring ProductivityActionsPlugin::ResolveFenceId(const CommandContext& command) const
{
    if (!command.fence.id.empty())
    {
        return command.fence.id;
    }

    if (m_context.appCommands)
    {
        const CommandContext active = m_context.appCommands->GetCurrentCommandContext();
        if (!active.fence.id.empty())
        {
            return active.fence.id;
        }

        return m_context.appCommands->GetActiveFenceMetadata().id;
    }

    return L"";
}

FenceMetadata ProductivityActionsPlugin::ResolveFence(const CommandContext& command) const
{
    if (!command.fence.id.empty())
    {
        return command.fence;
    }

    const std::wstring fenceId = ResolveFenceId(command);
    if (fenceId.empty() || !m_context.appCommands)
    {
        return {};
    }

    return m_context.appCommands->GetFenceMetadata(fenceId);
}

bool ProductivityActionsPlugin::GetBool(const std::wstring& key, bool fallback) const
{
    return GetString(key, fallback ? L"true" : L"false") == L"true";
}

bool ProductivityActionsPlugin::IsPluginEnabled() const
{
    return GetBool(L"plugin.enabled", true);
}

bool ProductivityActionsPlugin::ShouldLogActions() const
{
    return GetBool(L"plugin.log_actions", true);
}

bool ProductivityActionsPlugin::IsSafeModeEnabled() const
{
    return GetBool(L"plugin.safe_mode", true);
}

std::wstring ProductivityActionsPlugin::GetDefaultMode() const
{
    return GetString(L"plugin.default_mode", L"balanced");
}

int ProductivityActionsPlugin::GetRefreshIntervalSeconds() const
{
    int seconds = GetInt(L"plugin.refresh_interval_seconds", 120);
    if (seconds < 1)
    {
        seconds = 1;
    }
    return seconds;
}

void ProductivityActionsPlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[ProductivityActions][Notification] " + message);
}

void ProductivityActionsPlugin::RefreshFenceWithThrottle(const std::wstring& fenceId) const
{
    if (!m_context.appCommands || fenceId.empty())
    {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto interval = std::chrono::seconds(GetRefreshIntervalSeconds());
    if (m_lastRefreshAt.time_since_epoch().count() != 0 && (now - m_lastRefreshAt) < interval)
    {
        LogInfo(L"Refresh throttled by plugin.refresh_interval_seconds");
        return;
    }

    m_lastRefreshAt = now;
    m_context.appCommands->RefreshFence(fenceId);
}

int ProductivityActionsPlugin::GetInt(const std::wstring& key, int fallback) const
{
    const std::wstring text = GetString(key, std::to_wstring(fallback));
    try
    {
        return std::stoi(text);
    }
    catch (...)
    {
        return fallback;
    }
}

std::wstring ProductivityActionsPlugin::GetString(const std::wstring& key, const std::wstring& fallback) const
{
    return m_context.settingsRegistry ? m_context.settingsRegistry->GetValue(key, fallback) : fallback;
}

void ProductivityActionsPlugin::LogInfo(const std::wstring& message) const
{
    if (m_context.diagnostics && ShouldLogActions())
    {
        m_context.diagnostics->Info(L"[ProductivityActions] " + message);
    }
}

void ProductivityActionsPlugin::LogWarn(const std::wstring& message) const
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Warn(L"[ProductivityActions] " + message);
    }
}

fs::path ProductivityActionsPlugin::BuildUniquePath(const fs::path& target)
{
    if (!fs::exists(target))
    {
        return target;
    }

    const fs::path stem = target.stem();
    const fs::path ext = target.extension();
    const fs::path dir = target.parent_path();
    for (int i = 1; i < 1000; ++i)
    {
        const fs::path candidate = dir / (stem.wstring() + L" (" + std::to_wstring(i) + L")" + ext.wstring());
        if (!fs::exists(candidate))
        {
            return candidate;
        }
    }

    return target;
}


