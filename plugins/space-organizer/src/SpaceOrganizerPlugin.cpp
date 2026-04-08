#include "SpaceOrganizerPlugin.h"

#include "core/CommandDispatcher.h"
#include "core/Diagnostics.h"
#include "../../shared/SampleFsHelpers.h"
#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

PluginManifest SpaceOrganizerPlugin::GetManifest() const
{
    PluginManifest m;
    m.id = L"community.space_organizer";
    m.displayName = L"Space Organizer";
    m.version = L"1.2.2";
    m.description = L"Sorting and cleanup toolkit for space contents using context-aware commands.";
    m.minHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.capabilities = {L"commands", L"menu_contributions", L"settings_pages"};
    return m;
}

bool SpaceOrganizerPlugin::Initialize(const PluginContext& context)
{
    m_context = context;
    if (!m_context.commandDispatcher || !m_context.settingsRegistry || !m_context.appCommands)
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Error(L"SpaceOrganizerPlugin: Missing required host services");
        }
        return false;
    }

    RegisterSettings();
    RegisterMenus();
    RegisterCommands();
    LogInfo(L"Initialized");
    return true;
}

void SpaceOrganizerPlugin::Shutdown()
{
    LogInfo(L"Shutdown");
}

void SpaceOrganizerPlugin::RegisterSettings() const
{
    PluginSettingsPage page;
    page.pluginId = L"community.space_organizer";
    page.pageId = L"organizer.actions";
    page.title = L"Space Organizer";
    page.order = 30;

    page.fields.push_back(SettingsFieldDescriptor{L"plugin.enabled", L"Enable plugin", L"Master toggle for Space Organizer behavior.", SettingsFieldType::Bool, L"true", {}, 1});
    page.fields.push_back(SettingsFieldDescriptor{L"plugin.log_actions", L"Log actions", L"Write organizer operations to diagnostics.", SettingsFieldType::Bool, L"true", {}, 2});
    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 120, false);

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.actions.folder_prefix",
        L"Type bucket prefix",
        L"Prefix used for file-type folders created by Organize by Type.",
        SettingsFieldType::String,
        L"_type_",
        {},
        10});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.actions.include_hidden",
        L"Include hidden files",
        L"Include hidden files during organizer operations.",
        SettingsFieldType::Bool,
        L"false",
        {},
        20});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.actions.skip_shortcuts",
        L"Skip shortcuts (.lnk)",
        L"Ignore .lnk files when moving files between folders.",
        SettingsFieldType::Bool,
        L"true",
        {},
        30});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.actions.only_managed_prefix",
        L"Flatten only managed folders",
        L"Only flatten folders with the configured type bucket prefix.",
        SettingsFieldType::Bool,
        L"true",
        {},
        40});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.actions.archive_folder",
        L"Archive folder name",
        L"Folder name used for Archive Old Files.",
        SettingsFieldType::String,
        L"_archive",
        {},
        50});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.actions.large_files_folder",
        L"Large files folder name",
        L"Folder name used for Move Large Files.",
        SettingsFieldType::String,
        L"_large_files",
        {},
        60});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.analysis.large_file_threshold_mb",
        L"Large file threshold (MB)",
        L"Files at or above this size are moved by Move Large Files.",
        SettingsFieldType::Int,
        L"50",
        {},
        70});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.analysis.old_file_days",
        L"Old file threshold (days)",
        L"Files older than this are moved by Archive Old Files.",
        SettingsFieldType::Int,
        L"180",
        {},
        80});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.confirm.destructive_operations",
        L"Confirm destructive operations",
        L"Require explicit user confirmation before move or archive operations affect real files.",
        SettingsFieldType::Bool,
        L"true",
        {},
        90});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.confirm.skip_recycle_bin",
        L"Skip Recycle Bin for empty folder cleanup",
        L"Permanently delete empty folders without sending them to the Recycle Bin.",
        SettingsFieldType::Bool,
        L"false",
        {},
        100});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.confirm.skip_locked_files",
        L"Skip locked files",
        L"Silently skip files that are locked by another process rather than failing the operation.",
        SettingsFieldType::Bool,
        L"true",
        {},
        110});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.archive.date_subfolder_format",
        L"Archive date subfolder format",
        L"Date-based subfolder name appended inside the archive folder. Use strftime-style tokens: %Y, %m, %d.",
        SettingsFieldType::Enum,
        L"none",
        {
            {L"none", L"No date subfolder"},
            {L"year", L"Year (%Y)"},
            {L"year_month", L"Year-Month (%Y-%m)"},
            {L"year_month_day", L"Year-Month-Day (%Y-%m-%d)"},
        },
        120});

    page.fields.push_back(SettingsFieldDescriptor{
        L"organizer.archive.use_modified_time",
        L"Use modified time for archiving",
        L"Compare file modified time instead of created time when evaluating the old-file threshold.",
        SettingsFieldType::Bool,
        L"true",
        {},
        130});

    m_context.settingsRegistry->RegisterPage(std::move(page));
}

void SpaceOrganizerPlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Organize by File Type", L"organizer.by_type", 500, true});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Flatten Organized Folders", L"organizer.flatten", 510, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Remove Empty Subfolders", L"organizer.cleanup_empty", 520, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Archive Old Files", L"organizer.archive_old", 530, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Move Large Files", L"organizer.move_large", 540, false});
}

void SpaceOrganizerPlugin::RegisterCommands() const
{
    m_context.commandDispatcher->RegisterCommand(L"organizer.by_type", [this](const CommandContext& command) {
        HandleOrganizeByType(command);
    });
    m_context.commandDispatcher->RegisterCommand(L"organizer.flatten", [this](const CommandContext& command) {
        HandleFlatten(command);
    });
    m_context.commandDispatcher->RegisterCommand(L"organizer.cleanup_empty", [this](const CommandContext& command) {
        HandleCleanupEmpty(command);
    });
    m_context.commandDispatcher->RegisterCommand(L"organizer.archive_old", [this](const CommandContext& command) {
        HandleArchiveOld(command);
    });
    m_context.commandDispatcher->RegisterCommand(L"organizer.move_large", [this](const CommandContext& command) {
        HandleMoveLarge(command);
    });
}

SpaceMetadata SpaceOrganizerPlugin::ResolveSpace(const CommandContext& command) const
{
    if (!command.space.id.empty())
    {
        return command.space;
    }

    if (m_context.appCommands)
    {
        return m_context.appCommands->GetCurrentCommandContext().space;
    }

    return {};
}

bool SpaceOrganizerPlugin::GetBool(const std::wstring& key, const std::wstring& fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback == L"true";
    }

    return m_context.settingsRegistry->GetValue(key, fallback) == L"true";
}

int SpaceOrganizerPlugin::GetInt(const std::wstring& key, int fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    const std::wstring text = m_context.settingsRegistry->GetValue(key, std::to_wstring(fallback));
    try
    {
        return std::stoi(text);
    }
    catch (...)
    {
        return fallback;
    }
}

std::wstring SpaceOrganizerPlugin::GetText(const std::wstring& key, const std::wstring& fallback) const
{
    if (!m_context.settingsRegistry)
    {
        return fallback;
    }

    return m_context.settingsRegistry->GetValue(key, fallback);
}

bool SpaceOrganizerPlugin::IsPluginEnabled() const
{
    return GetBool(L"plugin.enabled", L"true");
}

bool SpaceOrganizerPlugin::ShouldLogActions() const
{
    return GetBool(L"plugin.log_actions", L"true");
}

bool SpaceOrganizerPlugin::IsSafeModeEnabled() const
{
    return GetBool(L"plugin.safe_mode", L"true");
}

std::wstring SpaceOrganizerPlugin::GetDefaultMode() const
{
    return GetText(L"plugin.default_mode", L"balanced");
}

int SpaceOrganizerPlugin::GetRefreshIntervalSeconds() const
{
    int seconds = GetInt(L"plugin.refresh_interval_seconds", 120);
    if (seconds < 1)
    {
        seconds = 1;
    }
    return seconds;
}

void SpaceOrganizerPlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", L"false") || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[SpaceOrganizer][Notification] " + message);
}

void SpaceOrganizerPlugin::RefreshSpaceWithThrottle(const std::wstring& spaceId) const
{
    if (!m_context.appCommands || spaceId.empty())
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
    m_context.appCommands->RefreshSpace(spaceId);
}

void SpaceOrganizerPlugin::LogInfo(const std::wstring& message) const
{
    if (m_context.diagnostics && ShouldLogActions())
    {
        m_context.diagnostics->Info(L"[SpaceOrganizer] " + message);
    }
}

void SpaceOrganizerPlugin::LogWarn(const std::wstring& message) const
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Warn(L"[SpaceOrganizer] " + message);
    }
}

std::wstring SpaceOrganizerPlugin::SanitizeExtension(const fs::path& path)
{
    std::wstring ext = path.extension().wstring();
    if (ext.empty())
    {
        return L"no_extension";
    }

    if (!ext.empty() && ext.front() == L'.')
    {
        ext.erase(ext.begin());
    }

    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    std::replace(ext.begin(), ext.end(), L' ', L'_');
    return ext.empty() ? L"no_extension" : ext;
}

void SpaceOrganizerPlugin::HandleOrganizeByType(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty() || space.backingFolderPath.empty())
    {
        LogWarn(L"No space context available for organize action.");
        return;
    }

    const bool includeHidden = GetBool(L"organizer.actions.include_hidden", L"false");
    const bool skipShortcuts = GetBool(L"organizer.actions.skip_shortcuts", L"true");
    std::wstring prefix = GetText(L"organizer.actions.folder_prefix", L"_type_");
    if (prefix.empty())
    {
        prefix = L"_type_";
    }

    size_t moved = 0;
    size_t skipped = 0;
    try
    {
        const fs::path root(space.backingFolderPath);
        for (const auto& entry : fs::directory_iterator(root))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            const fs::path file = entry.path();
            if (!includeHidden && SampleFsHelpers::IsHiddenPath(file))
            {
                ++skipped;
                continue;
            }
            if (skipShortcuts && file.extension() == L".lnk")
            {
                ++skipped;
                continue;
            }

            const fs::path bucket = root / (prefix + SanitizeExtension(file));
            fs::create_directories(bucket);
            fs::rename(file, SampleFsHelpers::BuildUniquePath(bucket / file.filename()));
            ++moved;
        }

        RefreshSpaceWithThrottle(space.id);
        LogInfo(L"Organize by type moved " + std::to_wstring(moved) + L" item(s), skipped " + std::to_wstring(skipped) + L".");
        Notify(L"Organize by type completed.");
    }
    catch (const std::exception&)
    {
        LogWarn(L"Organize by type failed due to filesystem exception.");
    }
}

void SpaceOrganizerPlugin::HandleFlatten(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty() || space.backingFolderPath.empty())
    {
        LogWarn(L"No space context available for flatten action.");
        return;
    }

    const bool includeHidden = GetBool(L"organizer.actions.include_hidden", L"false");
    const bool skipShortcuts = GetBool(L"organizer.actions.skip_shortcuts", L"true");
    const bool onlyManaged = GetBool(L"organizer.actions.only_managed_prefix", L"true");
    const std::wstring prefix = GetText(L"organizer.actions.folder_prefix", L"_type_");

    size_t moved = 0;
    try
    {
        const fs::path root(space.backingFolderPath);
        for (const auto& dirEntry : fs::directory_iterator(root))
        {
            if (!dirEntry.is_directory())
            {
                continue;
            }

            const fs::path folder = dirEntry.path();
            if (onlyManaged && !prefix.empty())
            {
                const std::wstring name = folder.filename().wstring();
                if (name.rfind(prefix, 0) != 0)
                {
                    continue;
                }
            }

            for (const auto& fileEntry : fs::directory_iterator(folder))
            {
                if (!fileEntry.is_regular_file())
                {
                    continue;
                }

                const fs::path file = fileEntry.path();
                if (!includeHidden && SampleFsHelpers::IsHiddenPath(file))
                {
                    continue;
                }
                if (skipShortcuts && file.extension() == L".lnk")
                {
                    continue;
                }

                fs::rename(file, SampleFsHelpers::BuildUniquePath(root / file.filename()));
                ++moved;
            }
        }

        RefreshSpaceWithThrottle(space.id);
        LogInfo(L"Flatten moved " + std::to_wstring(moved) + L" item(s).");
        Notify(L"Flatten organized folders completed.");
    }
    catch (const std::exception&)
    {
        LogWarn(L"Flatten failed due to filesystem exception.");
    }
}

void SpaceOrganizerPlugin::HandleCleanupEmpty(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty() || space.backingFolderPath.empty())
    {
        LogWarn(L"No space context available for cleanup action.");
        return;
    }

    size_t removed = 0;
    std::error_code ec;
    try
    {
        const fs::path root(space.backingFolderPath);
        for (const auto& entry : fs::directory_iterator(root))
        {
            if (entry.is_directory() && fs::is_empty(entry.path(), ec) && !ec)
            {
                fs::remove(entry.path(), ec);
                if (!ec)
                {
                    ++removed;
                }
            }
            ec.clear();
        }

        RefreshSpaceWithThrottle(space.id);
        LogInfo(L"Cleanup removed " + std::to_wstring(removed) + L" empty folder(s).");
        Notify(L"Cleanup removed empty folders.");
    }
    catch (const std::exception&)
    {
        LogWarn(L"Cleanup failed due to filesystem exception.");
    }
}

void SpaceOrganizerPlugin::HandleArchiveOld(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty() || space.backingFolderPath.empty())
    {
        LogWarn(L"No space context available for archive action.");
        return;
    }

    int days = GetInt(L"organizer.analysis.old_file_days", 180);
    if (IsSafeModeEnabled() && days < 30)
    {
        days = 30;
    }

    const std::wstring mode = GetDefaultMode();
    if (mode == L"aggressive" && days > 30)
    {
        days = 30;
    }
    else if (mode == L"conservative" && days < 180)
    {
        days = 180;
    }

    if (days < 1)
    {
        days = 1;
    }

    std::wstring archiveFolderName = GetText(L"organizer.actions.archive_folder", L"_archive");
    if (archiveFolderName.empty())
    {
        archiveFolderName = L"_archive";
    }

    const auto cutoff = fs::file_time_type::clock::now() - std::chrono::hours(24 * days);
    size_t moved = 0;
    try
    {
        const fs::path root(space.backingFolderPath);
        const fs::path archiveFolder = root / archiveFolderName;
        fs::create_directories(archiveFolder);

        for (const auto& entry : fs::directory_iterator(root))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            std::error_code ec;
            const auto writeTime = fs::last_write_time(entry.path(), ec);
            if (!ec && writeTime < cutoff)
            {
                fs::rename(entry.path(), SampleFsHelpers::BuildUniquePath(archiveFolder / entry.path().filename()));
                ++moved;
            }
        }

        RefreshSpaceWithThrottle(space.id);
        LogInfo(L"Archived " + std::to_wstring(moved) + L" old file(s).");
        Notify(L"Archive old files completed.");
    }
    catch (const std::exception&)
    {
        LogWarn(L"Archive old files failed due to filesystem exception.");
    }
}

void SpaceOrganizerPlugin::HandleMoveLarge(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty() || space.backingFolderPath.empty())
    {
        LogWarn(L"No space context available for large-file action.");
        return;
    }

    int thresholdMb = GetInt(L"organizer.analysis.large_file_threshold_mb", 50);
    if (IsSafeModeEnabled() && thresholdMb < 100)
    {
        thresholdMb = 100;
    }

    const std::wstring mode = GetDefaultMode();
    if (mode == L"aggressive" && thresholdMb > 25)
    {
        thresholdMb = 25;
    }
    else if (mode == L"conservative" && thresholdMb < 200)
    {
        thresholdMb = 200;
    }

    if (thresholdMb < 1)
    {
        thresholdMb = 1;
    }

    std::wstring largeFolderName = GetText(L"organizer.actions.large_files_folder", L"_large_files");
    if (largeFolderName.empty())
    {
        largeFolderName = L"_large_files";
    }

    const uintmax_t thresholdBytes = static_cast<uintmax_t>(thresholdMb) * 1024u * 1024u;
    size_t moved = 0;
    try
    {
        const fs::path root(space.backingFolderPath);
        const fs::path largeFolder = root / largeFolderName;
        fs::create_directories(largeFolder);

        for (const auto& entry : fs::directory_iterator(root))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            std::error_code ec;
            const uintmax_t size = fs::file_size(entry.path(), ec);
            if (!ec && size >= thresholdBytes)
            {
                fs::rename(entry.path(), SampleFsHelpers::BuildUniquePath(largeFolder / entry.path().filename()));
                ++moved;
            }
        }

        RefreshSpaceWithThrottle(space.id);
        LogInfo(L"Moved " + std::to_wstring(moved) + L" large file(s).");
        Notify(L"Move large files completed.");
    }
    catch (const std::exception&)
    {
        LogWarn(L"Move large files failed due to filesystem exception.");
    }
}



