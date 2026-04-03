#include "RulesEnginePlugin.h"

#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"
#include "../../shared/PluginUiPatterns.h"

#include <codecvt>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>

namespace
{
    std::wstring DetermineRoute(const FenceItemMetadata& item)
    {
        if (item.isDirectory)
        {
            return L"folders";
        }

        const auto pos = item.name.find_last_of(L'.');
        if (pos == std::wstring::npos || pos + 1 >= item.name.size())
        {
            return L"misc";
        }

        std::wstring ext = item.name.substr(pos + 1);
        for (auto& ch : ext)
        {
            ch = static_cast<wchar_t>(towlower(ch));
        }

        if (ext == L"png" || ext == L"jpg" || ext == L"jpeg" || ext == L"gif" || ext == L"webp")
        {
            return L"images";
        }
        if (ext == L"cpp" || ext == L"h" || ext == L"hpp" || ext == L"cs" || ext == L"ts" || ext == L"js" || ext == L"py")
        {
            return L"code";
        }
        if (ext == L"zip" || ext == L"7z" || ext == L"rar")
        {
            return L"archives";
        }

        return L"misc";
    }

    std::string ToUtf8(const std::wstring& text)
    {
        if (text.empty())
        {
            return {};
        }

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(text);
    }

    std::string EscapeJson(const std::wstring& text)
    {
        std::ostringstream out;
        for (const wchar_t ch : text)
        {
            switch (ch)
            {
            case L'\\': out << "\\\\"; break;
            case L'"': out << "\\\""; break;
            case L'\n': out << "\\n"; break;
            case L'\r': out << "\\r"; break;
            case L'\t': out << "\\t"; break;
            default:
                out << ToUtf8(std::wstring(1, ch));
                break;
            }
        }
        return out.str();
    }

    std::string UtcNowIso8601()
    {
        const std::time_t now = std::time(nullptr);
        std::tm utc{};
#if defined(_WIN32)
        gmtime_s(&utc, &now);
#else
        gmtime_r(&now, &utc);
#endif
        std::ostringstream out;
        out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
        return out.str();
    }
}

PluginManifest RulesEnginePlugin::GetManifest() const
{
    PluginManifest m;
    m.id = L"community.rules_engine";
    m.displayName = L"Rules Engine";
    m.version = L"1.1.1";
    m.description = L"Routes and classifies items using ordered matching rules.";
    m.minHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.maxHostApiVersion = SimpleFencesVersion::kPluginApiVersion;
    m.capabilities = {L"commands", L"settings_pages", L"tray_contributions"};
    return m;
}

bool RulesEnginePlugin::Initialize(const PluginContext& context)
{
    m_context = context;
    if (!m_context.commandDispatcher || !m_context.settingsRegistry)
    {
        if (m_context.diagnostics)
        {
            m_context.diagnostics->Error(L"RulesEnginePlugin: Missing required host services");
        }
        return false;
    }

    RegisterSettings();
    RegisterMenus();
    RegisterCommands();
    LogInfo(L"Initialized");
    return true;
}

void RulesEnginePlugin::Shutdown()
{
    LogInfo(L"Shutdown");
}

void RulesEnginePlugin::RegisterSettings() const
{
    PluginSettingsPage page;
    page.pluginId = L"community.rules_engine";
    page.pageId = L"rules.general";
    page.title = L"Rules Engine";
    page.order = 50;

    page.fields.push_back(SettingsFieldDescriptor{L"plugin.enabled", L"Enable plugin", L"Master toggle for Rules Engine behavior.", SettingsFieldType::Bool, L"true", {}, 1});
    page.fields.push_back(SettingsFieldDescriptor{L"plugin.log_actions", L"Log actions", L"Write rule evaluation actions to diagnostics.", SettingsFieldType::Bool, L"true", {}, 2});
    PluginUiPatterns::AppendBaselineSettingsFields(page.fields, 1, 60, false);

    page.fields.push_back(SettingsFieldDescriptor{L"rules.enabled", L"Enable rules", L"Master toggle for rules evaluation.", SettingsFieldType::Bool, L"true", {}, 10});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.eval_mode", L"Evaluation mode", L"How rules are evaluated when multiple match.", SettingsFieldType::Enum, L"first_match", {{L"first_match", L"First match"}, {L"all_matches", L"All matches"}, {L"first_terminal_match", L"First terminal match"}}, 20});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.log_matches", L"Log rule matches", L"Write rule outcomes to diagnostics log.", SettingsFieldType::Bool, L"true", {}, 30});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.dry_run", L"Dry run", L"Evaluate rules without applying mutations.", SettingsFieldType::Bool, L"false", {}, 40});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.debounce_ms", L"Debounce (ms)", L"Debounce interval for burst command triggers.", SettingsFieldType::Int, L"300", {}, 50});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.max_rules_per_item", L"Max rules per item", L"Safety cap for per-item evaluation steps.", SettingsFieldType::Int, L"100", {}, 60});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.prevent_loops", L"Prevent loops", L"Prevent repeated re-routing loops.", SettingsFieldType::Bool, L"true", {}, 70});

    page.fields.push_back(SettingsFieldDescriptor{L"rules.file.path", L"Rules file path", L"Path to the JSON rules file loaded by this plugin. Leave blank to use the built-in default location.", SettingsFieldType::String, L"", {}, 80});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.file.auto_reload", L"Auto-reload rules file", L"Watch the rules file for changes and reload automatically without restarting.", SettingsFieldType::Bool, L"true", {}, 90});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.file.schema_version", L"Expected schema version", L"Schema version string the rules loader validates against. Mismatch triggers a load warning.", SettingsFieldType::String, L"1", {}, 100});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.export.include_disabled", L"Export — include disabled rules", L"Include rules marked as disabled when exporting the rule set.", SettingsFieldType::Bool, L"true", {}, 110});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.export.format", L"Export format", L"File format used when exporting the rule set.", SettingsFieldType::Enum, L"json", {{L"json", L"JSON"}, {L"json_pretty", L"JSON (pretty-printed)"}}, 120});
    page.fields.push_back(SettingsFieldDescriptor{L"rules.notification.on_match", L"Notify on rule match", L"Emit a diagnostic notification each time a rule successfully routes an item.", SettingsFieldType::Bool, L"false", {}, 130});

    m_context.settingsRegistry->RegisterPage(std::move(page));
}

void RulesEnginePlugin::RegisterMenus() const
{
    if (!m_context.menuRegistry)
    {
        return;
    }

    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Run Rules Now", L"rules.run_now", 310, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Pause Rules", L"rules.pause_toggle", 320, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Open Rules Editor", L"rules.editor.open", 330, false});
    m_context.menuRegistry->Register(MenuContribution{MenuSurface::Tray, L"Export Rules", L"rules.export", 340, false});
}

void RulesEnginePlugin::RegisterCommands() const
{
    m_context.commandDispatcher->RegisterCommand(L"rules.run_now", [this](const CommandContext& command) { HandleRunNow(command); });
    m_context.commandDispatcher->RegisterCommand(L"rules.pause_toggle", [this](const CommandContext& command) { HandlePauseToggle(command); });
    m_context.commandDispatcher->RegisterCommand(L"rules.editor.open", [this](const CommandContext& command) { HandleEditorOpen(command); });
    m_context.commandDispatcher->RegisterCommand(L"rules.test_item", [this](const CommandContext& command) { HandleTestItem(command); });
    m_context.commandDispatcher->RegisterCommand(L"rules.export", [this](const CommandContext& command) { HandleExport(command); });
}

void RulesEnginePlugin::HandleRunNow(const CommandContext&) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    if (!CanRunNow())
    {
        return;
    }

    if (!GetBool(L"rules.enabled", true))
    {
        LogInfo(L"Run skipped because rules.enabled=false");
        return;
    }

    if (m_paused)
    {
        LogInfo(L"Run skipped because rules are paused");
        return;
    }

    const int maxRules = GetInt(L"rules.max_rules_per_item", 100);
    std::wstring mode = GetString(L"rules.eval_mode", L"");
    if (mode.empty())
    {
        const std::wstring defaultMode = GetDefaultMode();
        mode = (defaultMode == L"strict") ? L"first_terminal_match" : ((defaultMode == L"relaxed") ? L"all_matches" : L"first_match");
    }

    if (IsSafeModeEnabled())
    {
        mode = L"first_terminal_match";
    }

    LogInfo(L"Run now: eval_mode=" + mode + L", max_rules_per_item=" + std::to_wstring(maxRules));
    Notify(L"Rules run completed.");
}

void RulesEnginePlugin::HandlePauseToggle(const CommandContext&) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    m_paused = !m_paused;
    LogInfo(m_paused ? L"Rules paused" : L"Rules resumed");
    Notify(m_paused ? L"Rules engine paused." : L"Rules engine resumed.");
}

void RulesEnginePlugin::HandleEditorOpen(const CommandContext&) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    LogInfo(L"Rules editor open requested (sample plugin logs only)");
}

void RulesEnginePlugin::HandleTestItem(const CommandContext& command) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    CommandContext effective = command;
    if (!effective.item.has_value() && m_context.appCommands)
    {
        effective = m_context.appCommands->GetCurrentCommandContext();
    }

    if (!effective.item.has_value())
    {
        LogWarn(L"rules.test_item skipped because no item payload was provided");
        return;
    }

    const std::wstring route = DetermineRoute(*effective.item);
    if (GetBool(L"rules.log_matches", true))
    {
        LogInfo(L"rules.test_item -> route='" + route + L"' item='" + effective.item->name + L"'");
    }
}

void RulesEnginePlugin::HandleExport(const CommandContext&) const
{
    if (!IsPluginEnabled())
    {
        return;
    }

    if (!m_context.settingsRegistry)
    {
        return;
    }

    std::ofstream out("rules-engine-export.json", std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        LogWarn(L"rules.export failed to write rules-engine-export.json");
        return;
    }

    const std::wstring evalMode = GetString(L"rules.eval_mode", L"first_match");
    const bool enabled = GetBool(L"rules.enabled", true);
    bool dryRun = GetBool(L"rules.dry_run", false);
    if (IsSafeModeEnabled())
    {
        dryRun = true;
    }
    const bool preventLoops = GetBool(L"rules.prevent_loops", true);
    const int debounceMs = GetInt(L"rules.debounce_ms", 300);
    const int maxRulesPerItem = GetInt(L"rules.max_rules_per_item", 100);

    out << "{\n";
    out << "  \"schemaVersion\": \"1.0\",\n";
    out << "  \"schemaRef\": \"plugins/rules-engine/rules.schema.json\",\n";
    out << "  \"sampleRuleFile\": \"plugins/rules-engine/rules.sample.json\",\n";
    out << "  \"exportedAtUtc\": \"" << UtcNowIso8601() << "\",\n";
    out << "  \"settings\": {\n";
    out << "    \"enabled\": " << (enabled ? "true" : "false") << ",\n";
    out << "    \"evalMode\": \"" << EscapeJson(evalMode) << "\",\n";
    out << "    \"dryRun\": " << (dryRun ? "true" : "false") << ",\n";
    out << "    \"preventLoops\": " << (preventLoops ? "true" : "false") << ",\n";
    out << "    \"debounceMs\": " << debounceMs << ",\n";
    out << "    \"maxRulesPerItem\": " << maxRulesPerItem << "\n";
    out << "  },\n";
    out << "  \"sampleRules\": [\n";
    out << "    {\"id\":\"route-images\",\"enabled\":true,\"priority\":100,\"stopProcessing\":true,\"match\":{\"extensions\":[\"png\",\"jpg\",\"jpeg\",\"gif\",\"webp\"]},\"actions\":[{\"type\":\"route_to_group\",\"group\":\"images\"}]},\n";
    out << "    {\"id\":\"route-code\",\"enabled\":true,\"priority\":90,\"stopProcessing\":true,\"match\":{\"extensions\":[\"cpp\",\"h\",\"hpp\",\"cs\",\"ts\",\"js\",\"py\"]},\"actions\":[{\"type\":\"route_to_group\",\"group\":\"code\"}]},\n";
    out << "    {\"id\":\"route-folders\",\"enabled\":true,\"priority\":80,\"stopProcessing\":true,\"match\":{\"isDirectory\":true},\"actions\":[{\"type\":\"route_to_group\",\"group\":\"folders\"}]}\n";
    out << "  ]\n";
    out << "}\n";

    out.close();
    LogInfo(L"rules.export wrote rules-engine-export.json with schemaVersion=1.0");
    Notify(L"Rules export completed.");
}

bool RulesEnginePlugin::GetBool(const std::wstring& key, bool fallback) const
{
    return GetString(key, fallback ? L"true" : L"false") == L"true";
}

bool RulesEnginePlugin::IsPluginEnabled() const
{
    return GetBool(L"plugin.enabled", true);
}

bool RulesEnginePlugin::ShouldLogActions() const
{
    return GetBool(L"plugin.log_actions", true);
}

bool RulesEnginePlugin::IsSafeModeEnabled() const
{
    return GetBool(L"plugin.safe_mode", true);
}

std::wstring RulesEnginePlugin::GetDefaultMode() const
{
    return GetString(L"plugin.default_mode", L"balanced");
}

int RulesEnginePlugin::GetRefreshIntervalSeconds() const
{
    int seconds = GetInt(L"plugin.refresh_interval_seconds", 60);
    if (seconds < 1)
    {
        seconds = 1;
    }
    return seconds;
}

void RulesEnginePlugin::Notify(const std::wstring& message) const
{
    if (!GetBool(L"plugin.show_notifications", false) || !m_context.diagnostics)
    {
        return;
    }

    m_context.diagnostics->Info(L"[RulesEngine][Notification] " + message);
}

bool RulesEnginePlugin::CanRunNow() const
{
    const auto now = std::chrono::steady_clock::now();
    const auto interval = std::chrono::seconds(GetRefreshIntervalSeconds());
    if (m_lastRunAt.time_since_epoch().count() != 0 && (now - m_lastRunAt) < interval)
    {
        LogInfo(L"rules.run_now throttled by plugin.refresh_interval_seconds");
        return false;
    }

    m_lastRunAt = now;
    return true;
}

int RulesEnginePlugin::GetInt(const std::wstring& key, int fallback) const
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

std::wstring RulesEnginePlugin::GetString(const std::wstring& key, const std::wstring& fallback) const
{
    return m_context.settingsRegistry ? m_context.settingsRegistry->GetValue(key, fallback) : fallback;
}

void RulesEnginePlugin::LogInfo(const std::wstring& message) const
{
    if (m_context.diagnostics && ShouldLogActions())
    {
        m_context.diagnostics->Info(L"[RulesEngine] " + message);
    }
}

void RulesEnginePlugin::LogWarn(const std::wstring& message) const
{
    if (m_context.diagnostics)
    {
        m_context.diagnostics->Warn(L"[RulesEngine] " + message);
    }
}


