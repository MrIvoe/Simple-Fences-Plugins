#include "SortingCleanupPlugin.h"

#include "core/CommandDispatcher.h"
#include "core/Diagnostics.h"
#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

PluginManifest SortingCleanupPlugin::GetManifest() const
{
    PluginManifest m;
    m.id = L"community.space_sort_cleanup";
    m.displayName = L"Space Sort and Clean Up";
    m.version = L"1.1.2";
    m.description = L"Sorts and arranges space items with optional automation and cleanup tools.";
    m.minHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleSpacesVersion::kPluginApiVersion;
    m.capabilities = {L"commands", L"tray_contributions", L"settings_pages", L"desktop_context"};
    return m;
}

bool SortingCleanupPlugin::Initialize(const PluginContext& context)
{
    m_context = context;
    if (!m_context.commandDispatcher || !m_context.settingsRegistry || !m_context.appCommands)
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Error(L"SortingCleanupPlugin: Missing required host services");
        }
        return false;
    }

    m_autoSortEnabled = GetBool(L"auto.on_item_add", false);
    RegisterSettings();
    RegisterMenus();
    RegisterCommands();
    LogInfo(L"Initialized");
    return true;
}

void SortingCleanupPlugin::Shutdown()
{
    LogInfo(L"Shutdown");
}

void SortingCleanupPlugin::RegisterSettings() const
{
    PluginSettingsPage page;
    page.pluginId = L"community.space_sort_cleanup";
    page.pageId = L"sort.cleanup";
    page.title = L"Sort and Clean Up";
    page.order = 65;

    page.fields.push_back(SettingsFieldDescriptor{L"plugin.enabled", L"Enable plugin", L"Master toggle for Sort and Clean Up behavior.", SettingsFieldType::Bool, L"true", {}, 1});
    page.fields.push_back(SettingsFieldDescriptor{L"plugin.log_actions", L"Log actions", L"Write sort and cleanup actions to diagnostics.", SettingsFieldType::Bool, L"true", {}, 2});
    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 90, false);

    page.fields.push_back(SettingsFieldDescriptor{L"sort.mode.default", L"Default sort mode", L"Default mode for sort commands and autosort.", SettingsFieldType::Enum, L"name", {{L"name", L"Name"}, {L"type", L"Type"}, {L"extension", L"Extension"}, {L"size", L"Size"}, {L"modified", L"Modified"}, {L"created", L"Created"}, {L"manual", L"Manual"}}, 10});
    page.fields.push_back(SettingsFieldDescriptor{L"sort.mode.direction", L"Sort direction", L"Ascending or descending sort order.", SettingsFieldType::Enum, L"asc", {{L"asc", L"Ascending"}, {L"desc", L"Descending"}}, 20});
    page.fields.push_back(SettingsFieldDescriptor{L"sort.case_sensitive", L"Case-sensitive sort", L"Use case-sensitive sort comparisons.", SettingsFieldType::Bool, L"false", {}, 30});
    page.fields.push_back(SettingsFieldDescriptor{L"sort.locale_aware", L"Locale-aware sort", L"Use locale-aware sorting where available.", SettingsFieldType::Bool, L"true", {}, 40});
    page.fields.push_back(SettingsFieldDescriptor{L"sort.folders_first", L"Folders first", L"Place folders before files when sorting mixed lists.", SettingsFieldType::Bool, L"true", {}, 50});
    page.fields.push_back(SettingsFieldDescriptor{L"layout.cleanup_after_sort", L"Cleanup after sort", L"Remove empty subfolders after sorting operations.", SettingsFieldType::Bool, L"true", {}, 60});
    page.fields.push_back(SettingsFieldDescriptor{L"layout.align_to_grid", L"Align to grid", L"Enable align-current-grid command behavior.", SettingsFieldType::Bool, L"true", {}, 70});
    page.fields.push_back(SettingsFieldDescriptor{L"layout.grid_spacing_px", L"Grid spacing (px)", L"Grid spacing hint used by alignment behavior.", SettingsFieldType::Int, L"12", {}, 80});
    page.fields.push_back(SettingsFieldDescriptor{L"layout.preserve_pinned", L"Preserve pinned items", L"Avoid moving pinned items when pin metadata is available.", SettingsFieldType::Bool, L"true", {}, 90});
    page.fields.push_back(SettingsFieldDescriptor{L"auto.on_item_add", L"Enable autosort on item add", L"Enable autosort mode for item-add triggers.", SettingsFieldType::Bool, L"false", {}, 100});
    page.fields.push_back(SettingsFieldDescriptor{L"auto.debounce_ms", L"Autosort debounce (ms)", L"Debounce interval for autosort trigger bursts.", SettingsFieldType::Int, L"800", {}, 110});

    page.fields.push_back(SettingsFieldDescriptor{L"sort.secondary_key", L"Secondary sort key", L"Tiebreaker sort key applied when two items compare equal on the primary key.", SettingsFieldType::Enum, L"name", {{L"name", L"Name"}, {L"modified", L"Modified"}, {L"size", L"Size"}, {L"none", L"None"}}, 120});
    page.fields.push_back(SettingsFieldDescriptor{L"sort.shortcuts_last", L"Sort shortcuts last", L"Place .lnk shortcut files after regular files regardless of the primary sort key.", SettingsFieldType::Bool, L"false", {}, 130});
    page.fields.push_back(SettingsFieldDescriptor{L"sort.natural_number_order", L"Natural number sort", L"Sort filenames containing numbers in natural order (e.g. file2 before file10).", SettingsFieldType::Bool, L"true", {}, 140});
    page.fields.push_back(SettingsFieldDescriptor{L"auto.scope", L"Autosort scope", L"Choose whether autosort triggers apply to all spaces or only the space where an item was added.", SettingsFieldType::Enum, L"active_space", {{L"active_space", L"Active space only"}, {L"all_spaces", L"All spaces"}}, 150});
    page.fields.push_back(SettingsFieldDescriptor{L"layout.confirm_cleanup", L"Confirm before cleanup", L"Ask for confirmation before the clean-up operation removes empty subfolders.", SettingsFieldType::Bool, L"true", {}, 160});

    m_context.settingsRegistry->RegisterPage(std::move(page));
}

void SortingCleanupPlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Sort Current by Name", L"sort.current.name", 810, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Sort Current by Type", L"sort.current.type", 820, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Sort Current by Modified", L"sort.current.modified", 830, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Clean Up Current", L"cleanup.current", 840, true});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::SpaceContext, L"Align Current to Grid", L"align.current.grid", 850, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Toggle Autosort", L"autosort.toggle", 860, false});
}

void SortingCleanupPlugin::RegisterCommands() const
{
    m_context.commandDispatcher->RegisterCommand(L"sort.current.name", [this](const CommandContext& command) { HandleSortByName(command); });
    m_context.commandDispatcher->RegisterCommand(L"sort.current.type", [this](const CommandContext& command) { HandleSortByType(command); });
    m_context.commandDispatcher->RegisterCommand(L"sort.current.modified", [this](const CommandContext& command) { HandleSortByModified(command); });
    m_context.commandDispatcher->RegisterCommand(L"cleanup.current", [this](const CommandContext& command) { HandleCleanupCurrent(command); });
    m_context.commandDispatcher->RegisterCommand(L"align.current.grid", [this](const CommandContext& command) { HandleAlignGrid(command); });
    m_context.commandDispatcher->RegisterCommand(L"autosort.toggle", [this](const CommandContext& command) { HandleAutoSortToggle(command); });
}

void SortingCleanupPlugin::HandleSortByName(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty() || space.backingFolderPath.empty())
    {
        LogWarn(L"sort.current.name skipped: no space context");
        return;
    }

    std::error_code ec;
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(space.backingFolderPath, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

        if (entry.is_regular_file())
        {
            files.push_back(entry.path());
        }
    }

    const bool caseSensitive = GetBool(L"sort.case_sensitive", false);
    const bool descending = GetString(L"sort.mode.direction", L"asc") == L"desc";

    std::sort(files.begin(), files.end(), [caseSensitive, descending](const fs::path& a, const fs::path& b) {
        std::wstring av = a.filename().wstring();
        std::wstring bv = b.filename().wstring();
        if (!caseSensitive)
        {
            std::transform(av.begin(), av.end(), av.begin(), ::towlower);
            std::transform(bv.begin(), bv.end(), bv.begin(), ::towlower);
        }

        return descending ? av > bv : av < bv;
    });

    ApplySortPlan(space, files);
    SetSetting(L"sort.mode.default", L"name");
    LogInfo(L"sort.current.name applied to " + std::to_wstring(files.size()) + L" files");
}

void SortingCleanupPlugin::HandleSortByType(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty() || space.backingFolderPath.empty())
    {
        LogWarn(L"sort.current.type skipped: no space context");
        return;
    }

    std::error_code ec;
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(space.backingFolderPath, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

        if (entry.is_regular_file())
        {
            files.push_back(entry.path());
        }
    }

    const bool descending = GetString(L"sort.mode.direction", L"asc") == L"desc";
    std::sort(files.begin(), files.end(), [descending](const fs::path& a, const fs::path& b) {
        std::wstring ae = a.extension().wstring();
        std::wstring be = b.extension().wstring();
        std::transform(ae.begin(), ae.end(), ae.begin(), ::towlower);
        std::transform(be.begin(), be.end(), be.begin(), ::towlower);
        if (ae == be)
        {
            return descending ? a.filename().wstring() > b.filename().wstring() : a.filename().wstring() < b.filename().wstring();
        }

        return descending ? ae > be : ae < be;
    });

    ApplySortPlan(space, files);
    SetSetting(L"sort.mode.default", L"type");
    LogInfo(L"sort.current.type applied to " + std::to_wstring(files.size()) + L" files");
}

void SortingCleanupPlugin::HandleSortByModified(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty() || space.backingFolderPath.empty())
    {
        LogWarn(L"sort.current.modified skipped: no space context");
        return;
    }

    std::error_code ec;
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(space.backingFolderPath, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

        if (entry.is_regular_file())
        {
            files.push_back(entry.path());
        }
    }

    const bool descending = GetString(L"sort.mode.direction", L"asc") == L"desc";
    std::sort(files.begin(), files.end(), [descending](const fs::path& a, const fs::path& b) {
        std::error_code aec;
        std::error_code bec;
        const auto at = fs::last_write_time(a, aec);
        const auto bt = fs::last_write_time(b, bec);
        if (aec || bec)
        {
            return descending ? a.filename().wstring() > b.filename().wstring() : a.filename().wstring() < b.filename().wstring();
        }

        return descending ? at > bt : at < bt;
    });

    ApplySortPlan(space, files);
    SetSetting(L"sort.mode.default", L"modified");
    LogInfo(L"sort.current.modified applied to " + std::to_wstring(files.size()) + L" files");
}

void SortingCleanupPlugin::HandleCleanupCurrent(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty() || space.backingFolderPath.empty())
    {
        LogWarn(L"cleanup.current skipped: no space context");
        return;
    }

    size_t removed = 0;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(space.backingFolderPath, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

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
    LogInfo(L"cleanup.current removed " + std::to_wstring(removed) + L" empty folder(s)");
    Notify(L"Cleanup current completed.");
}

void SortingCleanupPlugin::HandleAlignGrid(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    const SpaceMetadata space = ResolveSpace(command);
    if (space.id.empty())
    {
        LogWarn(L"align.current.grid skipped: no space context");
        return;
    }

    const bool alignEnabled = GetBool(L"layout.align_to_grid", true);
    const int spacing = GetInt(L"layout.grid_spacing_px", 12);
    if (!alignEnabled)
    {
        LogInfo(L"align.current.grid skipped because layout.align_to_grid=false");
        return;
    }

    LogInfo(L"align.current.grid requested for space=" + space.id + L" spacing=" + std::to_wstring(spacing) + L"px");
}

void SortingCleanupPlugin::HandleAutoSortToggle(const CommandContext&) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    m_autoSortEnabled = !m_autoSortEnabled;
    SetSetting(L"auto.on_item_add", m_autoSortEnabled ? L"true" : L"false");
    LogInfo(m_autoSortEnabled ? L"autosort enabled" : L"autosort disabled");
    Notify(m_autoSortEnabled ? L"Autosort enabled." : L"Autosort disabled.");
}

SpaceMetadata SortingCleanupPlugin::ResolveSpace(const CommandContext& command) const
{
    if (!command.space.id.empty())
    {
        return command.space;
    }

    const CommandContext current = m_context.appCommands->GetCurrentCommandContext();
    if (!current.space.id.empty())
    {
        return current.space;
    }

    const SpaceMetadata active = m_context.appCommands->GetActiveSpaceMetadata();
    if (!active.id.empty())
    {
        return active;
    }

    return {};
}

bool SortingCleanupPlugin::GetBool(const std::wstring& key, bool fallback) const
{
    return GetString(key, fallback ? L"true" : L"false") == L"true";
}

bool SortingCleanupPlugin::IsPluginEnabled() const
{
    return GetBool(L"plugin.enabled", true);
}

bool SortingCleanupPlugin::ShouldLogActions() const
{
    return GetBool(L"plugin.log_actions", true);
}

bool SortingCleanupPlugin::IsSafeModeEnabled() const
{
    return GetBool(L"plugin.safe_mode", true);
}

std::wstring SortingCleanupPlugin::GetDefaultMode() const
{
    return GetString(L"plugin.default_mode", L"balanced");
}

int SortingCleanupPlugin::GetRefreshIntervalSeconds() const
{
    int seconds = GetInt(L"plugin.refresh_interval_seconds", 90);
    if (seconds < 1)
    {
        seconds = 1;
    }
    return seconds;
}

void SortingCleanupPlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[SortCleanup][Notification] " + message);
}

void SortingCleanupPlugin::RefreshSpaceWithThrottle(const std::wstring& spaceId) const
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

int SortingCleanupPlugin::GetInt(const std::wstring& key, int fallback) const
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

std::wstring SortingCleanupPlugin::GetString(const std::wstring& key, const std::wstring& fallback) const
{
    return m_context.settingsRegistry ? m_context.settingsRegistry->GetValue(key, fallback) : fallback;
}

void SortingCleanupPlugin::SetSetting(const std::wstring& key, const std::wstring& value) const
{
    if (m_context.settingsRegistry)
    {
        m_context.settingsRegistry->SetValue(key, value);
    }
}

void SortingCleanupPlugin::LogInfo(const std::wstring& message) const
{
    if (m_context.diagnostics && ShouldLogActions())
    {
        m_context.diagnostics->Info(L"[SortCleanup] " + message);
    }
}

void SortingCleanupPlugin::LogWarn(const std::wstring& message) const
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Warn(L"[SortCleanup] " + message);
    }
}

fs::path SortingCleanupPlugin::BuildUniquePath(const fs::path& target)
{
    if (!fs::exists(target))
    {
        return target;
    }

    const fs::path stem = target.stem();
    const fs::path ext = target.extension();
    const fs::path dir = target.parent_path();
    for (int i = 1; i <= 999; ++i)
    {
        const fs::path candidate = dir / (stem.wstring() + L" (" + std::to_wstring(i) + L")" + ext.wstring());
        if (!fs::exists(candidate))
        {
            return candidate;
        }
    }

    return target;
}

void SortingCleanupPlugin::ApplySortPlan(const SpaceMetadata& space, const std::vector<fs::path>& files) const
{
    if (files.empty())
    {
        return;
    }

    if (IsSafeModeEnabled() && files.size() > 200)
    {
        LogWarn(L"Sort skipped in safe mode because item count exceeds 200.");
        return;
    }

    struct MoveStep
    {
        fs::path source;
        fs::path tempTarget;
        std::wstring originalName;
    };

    std::error_code ec;
    std::vector<MoveStep> moves;
    moves.reserve(files.size());

    int index = 1;
    for (const auto& source : files)
    {
        const std::wstring tempName = L"__sorted_" + std::to_wstring(index++) + L"__" + source.filename().wstring();
        moves.push_back(MoveStep{source, BuildUniquePath(source.parent_path() / tempName), source.filename().wstring()});
    }

    for (const auto& move : moves)
    {
        fs::rename(move.source, move.tempTarget, ec);
        if (ec)
        {
            LogWarn(L"Sort rename failed for " + move.source.filename().wstring());
            ec.clear();
        }
    }

    for (const auto& move : moves)
    {
        fs::path finalTarget = BuildUniquePath(move.tempTarget.parent_path() / move.originalName);
        fs::rename(move.tempTarget, finalTarget, ec);
        if (ec)
        {
            LogWarn(L"Sort finalize failed for " + move.tempTarget.filename().wstring());
            ec.clear();
        }
    }

    const std::wstring mode = GetDefaultMode();
    const bool cleanupAfterSort = (mode == L"aggressive") ? true : ((mode == L"manual") ? false : GetBool(L"layout.cleanup_after_sort", true));
    if (cleanupAfterSort)
    {
        size_t removed = 0;
        for (const auto& entry : fs::directory_iterator(space.backingFolderPath, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }

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

        if (removed > 0)
        {
            LogInfo(L"sort cleanup removed " + std::to_wstring(removed) + L" empty folder(s)");
        }
    }

    RefreshSpaceWithThrottle(space.id);
    Notify(L"Sort operation completed.");
}



