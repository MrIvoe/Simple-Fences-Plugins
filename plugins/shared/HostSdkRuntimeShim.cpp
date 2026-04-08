#include "core/CommandDispatcher.h"
#include "core/Diagnostics.h"
#include "core/SettingsStore.h"
#include "extensions/MenuContributionRegistry.h"
#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SpaceExtensionRegistry.h"

#include <algorithm>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#endif

void Diagnostics::Info(const std::wstring& message) const
{
#ifdef _WIN32
    const std::wstring line = L"[plugin][info] " + message + L"\n";
    OutputDebugStringW(line.c_str());
#else
    (void)message;
#endif
}

void Diagnostics::Warn(const std::wstring& message) const
{
#ifdef _WIN32
    const std::wstring line = L"[plugin][warn] " + message + L"\n";
    OutputDebugStringW(line.c_str());
#else
    (void)message;
#endif
}

void Diagnostics::Error(const std::wstring& message) const
{
#ifdef _WIN32
    const std::wstring line = L"[plugin][error] " + message + L"\n";
    OutputDebugStringW(line.c_str());
#else
    (void)message;
#endif
}

bool CommandDispatcher::RegisterCommand(const std::wstring& commandId, CommandHandler handler, bool replaceExisting)
{
    if (commandId.empty() || !handler)
    {
        return false;
    }

    const auto it = m_handlers.find(commandId);
    if (it != m_handlers.end() && !replaceExisting)
    {
        return false;
    }

    m_handlers[commandId] = std::move(handler);
    return true;
}

bool CommandDispatcher::RegisterCommand(const std::wstring& commandId, std::function<void()> handler, bool replaceExisting)
{
    if (!handler)
    {
        return false;
    }

    return RegisterCommand(commandId, [handler = std::move(handler)](const CommandContext&) {
        handler();
    }, replaceExisting);
}

bool CommandDispatcher::Dispatch(const std::wstring& commandId) const
{
    return DispatchDetailed(commandId).succeeded;
}

bool CommandDispatcher::Dispatch(const std::wstring& commandId, const CommandContext& context) const
{
    return DispatchDetailed(commandId, context).succeeded;
}

CommandDispatchResult CommandDispatcher::DispatchDetailed(const std::wstring& commandId) const
{
    CommandContext context;
    context.commandId = commandId;
    return DispatchDetailed(commandId, context);
}

CommandDispatchResult CommandDispatcher::DispatchDetailed(const std::wstring& commandId, const CommandContext& context) const
{
    CommandDispatchResult result;

    const auto it = m_handlers.find(commandId);
    if (it == m_handlers.end())
    {
        return result;
    }

    result.handled = true;

    try
    {
        CommandContext effectiveContext = context;
        if (effectiveContext.commandId.empty())
        {
            effectiveContext.commandId = commandId;
        }
        it->second(effectiveContext);
        result.succeeded = true;
    }
    catch (...)
    {
        result.error = L"Command threw exception.";
    }

    return result;
}

bool CommandDispatcher::HasCommand(const std::wstring& commandId) const
{
    return m_handlers.find(commandId) != m_handlers.end();
}

bool CommandDispatcher::UnregisterCommand(const std::wstring& commandId)
{
    return m_handlers.erase(commandId) > 0;
}

std::vector<std::wstring> CommandDispatcher::ListCommandIds() const
{
    std::vector<std::wstring> ids;
    ids.reserve(m_handlers.size());
    for (const auto& [id, _] : m_handlers)
    {
        (void)_;
        ids.push_back(id);
    }
    return ids;
}

bool MenuContributionRegistry::Register(const MenuContribution& contribution)
{
    if (contribution.title.empty() || contribution.commandId.empty())
    {
        return false;
    }

    for (const auto& existing : m_contributions)
    {
        if (existing.surface == contribution.surface &&
            existing.title == contribution.title &&
            existing.commandId == contribution.commandId)
        {
            return false;
        }
    }

    m_contributions.push_back(contribution);
    return true;
}

std::vector<MenuContribution> MenuContributionRegistry::GetBySurface(MenuSurface surface) const
{
    std::vector<MenuContribution> items;
    for (const auto& contribution : m_contributions)
    {
        if (contribution.surface == surface)
        {
            items.push_back(contribution);
        }
    }

    std::sort(items.begin(), items.end(), [](const MenuContribution& a, const MenuContribution& b) {
        if (a.order == b.order)
        {
            return a.title < b.title;
        }
        return a.order < b.order;
    });

    return items;
}

void MenuContributionRegistry::Clear()
{
    m_contributions.clear();
}

void PluginSettingsRegistry::SetStore(SettingsStore* store)
{
    m_store = store;
}

bool PluginSettingsRegistry::RegisterPage(const PluginSettingsPage& page)
{
    if (page.pluginId.empty() || page.pageId.empty() || page.title.empty())
    {
        return false;
    }

    for (auto& existing : m_pages)
    {
        if (existing.pluginId == page.pluginId && existing.pageId == page.pageId)
        {
            existing = page;
            return true;
        }
    }

    m_pages.push_back(page);
    return true;
}

std::vector<PluginSettingsPage> PluginSettingsRegistry::GetAllPages() const
{
    std::vector<PluginSettingsPage> pages = m_pages;
    std::sort(pages.begin(), pages.end(), [](const PluginSettingsPage& a, const PluginSettingsPage& b) {
        if (a.order == b.order)
        {
            return a.title < b.title;
        }
        return a.order < b.order;
    });
    return pages;
}

void PluginSettingsRegistry::ClearPages()
{
    m_pages.clear();
}

std::wstring PluginSettingsRegistry::GetValue(const std::wstring& key, const std::wstring& defaultValue) const
{
    if (m_store)
    {
        return m_store->Get(key, defaultValue);
    }

    const auto it = m_memValues.find(key);
    return (it != m_memValues.end()) ? it->second : defaultValue;
}

void PluginSettingsRegistry::SetValue(const std::wstring& key, const std::wstring& value)
{
    if (m_store)
    {
        m_store->Set(key, value);
    }
    else
    {
        m_memValues[key] = value;
    }

    for (const auto& [token, observer] : m_observers)
    {
        (void)token;
        if (observer)
        {
            observer(key, value);
        }
    }
}

int PluginSettingsRegistry::RegisterObserver(SettingsObserver observer)
{
    const int token = m_nextObserverToken++;
    m_observers[token] = std::move(observer);
    return token;
}

void PluginSettingsRegistry::UnregisterObserver(int observerToken)
{
    m_observers.erase(observerToken);
}

SpaceExtensionRegistry::SpaceExtensionRegistry()
{
    RegisteredProvider core;
    core.descriptor.providerId = L"core.file_collection";
    core.descriptor.contentType = L"file_collection";
    core.descriptor.displayName = L"File Collection";
    core.descriptor.isCoreDefault = true;
    m_contentProviders.push_back(std::move(core));
}

void SpaceExtensionRegistry::RegisterContentProvider(const SpaceContentProviderDescriptor& provider)
{
    RegisterContentProvider(provider, {});
}

void SpaceExtensionRegistry::RegisterContentProvider(const SpaceContentProviderDescriptor& provider, const SpaceContentProviderCallbacks& callbacks)
{
    if (provider.contentType.empty() || provider.providerId.empty())
    {
        return;
    }

    for (auto& existing : m_contentProviders)
    {
        if (existing.descriptor.contentType == provider.contentType &&
            existing.descriptor.providerId == provider.providerId)
        {
            existing.descriptor = provider;
            existing.callbacks = callbacks;
            return;
        }
    }

    RegisteredProvider entry;
    entry.descriptor = provider;
    entry.callbacks = callbacks;
    m_contentProviders.push_back(std::move(entry));
}

std::vector<SpaceContentProviderDescriptor> SpaceExtensionRegistry::GetContentProviders() const
{
    std::vector<SpaceContentProviderDescriptor> providers;
    providers.reserve(m_contentProviders.size());
    for (const auto& entry : m_contentProviders)
    {
        providers.push_back(entry.descriptor);
    }
    return providers;
}

bool SpaceExtensionRegistry::HasProvider(const std::wstring& contentType, const std::wstring& providerId) const
{
    for (const auto& entry : m_contentProviders)
    {
        if (entry.descriptor.contentType == contentType && entry.descriptor.providerId == providerId)
        {
            return true;
        }
    }
    return false;
}

SpaceContentProviderDescriptor SpaceExtensionRegistry::ResolveOrDefault(const std::wstring& contentType, const std::wstring& providerId) const
{
    for (const auto& entry : m_contentProviders)
    {
        if (entry.descriptor.contentType == contentType && entry.descriptor.providerId == providerId)
        {
            return entry.descriptor;
        }
    }

    for (const auto& entry : m_contentProviders)
    {
        if (entry.descriptor.contentType == contentType)
        {
            return entry.descriptor;
        }
    }

    return DefaultFileCollectionProvider();
}

const SpaceContentProviderCallbacks* SpaceExtensionRegistry::ResolveCallbacks(const std::wstring& contentType, const std::wstring& providerId) const
{
    for (const auto& entry : m_contentProviders)
    {
        if (entry.descriptor.contentType == contentType && entry.descriptor.providerId == providerId)
        {
            return &entry.callbacks;
        }
    }

    for (const auto& entry : m_contentProviders)
    {
        if (entry.descriptor.contentType == contentType)
        {
            return &entry.callbacks;
        }
    }

    return nullptr;
}

SpaceContentProviderDescriptor SpaceExtensionRegistry::DefaultFileCollectionProvider() const
{
    SpaceContentProviderDescriptor provider;
    provider.providerId = L"core.file_collection";
    provider.contentType = L"file_collection";
    provider.displayName = L"File Collection";
    provider.isCoreDefault = true;
    return provider;
}

bool SettingsStore::Load(const std::filesystem::path& filePath)
{
    m_filePath = filePath;
    return true;
}

bool SettingsStore::Save()
{
    return true;
}

std::wstring SettingsStore::Get(const std::wstring& key, const std::wstring& defaultValue) const
{
    const auto it = m_values.find(key);
    return (it != m_values.end()) ? it->second : defaultValue;
}

void SettingsStore::Set(const std::wstring& key, const std::wstring& value)
{
    m_values[key] = value;
}

bool SettingsStore::GetBool(const std::wstring& key, bool defaultValue) const
{
    const auto it = m_values.find(key);
    if (it == m_values.end())
    {
        return defaultValue;
    }
    return it->second == L"true";
}

void SettingsStore::SetBool(const std::wstring& key, bool value)
{
    Set(key, value ? L"true" : L"false");
}

int SettingsStore::GetInt(const std::wstring& key, int defaultValue) const
{
    const auto it = m_values.find(key);
    if (it == m_values.end())
    {
        return defaultValue;
    }

    try
    {
        return std::stoi(it->second);
    }
    catch (...)
    {
        return defaultValue;
    }
}

void SettingsStore::SetInt(const std::wstring& key, int value)
{
    Set(key, std::to_wstring(value));
}
