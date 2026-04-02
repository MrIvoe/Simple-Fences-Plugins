#include "TemplatePlugin.h"

#include "extensions/PluginSettingsRegistry.h"
#include "extensions/SettingsSchema.h"

PluginManifest TemplatePlugin::GetManifest() const
{
    PluginManifest manifest;
    manifest.id           = L"community.template_plugin";
    manifest.displayName  = L"Template Plugin";
    manifest.version      = L"0.1.0";
    manifest.description  = L"Starter template for a SimpleFences community plugin.";
    manifest.capabilities = {L"settings_pages"};
    return manifest;
}

bool TemplatePlugin::Initialize(const PluginContext& context)
{
    if (!context.settingsRegistry)
    {
        return true;
    }

    PluginSettingsPage page;
    page.pluginId = L"community.template_plugin";
    page.pageId   = L"template.general";
    page.title    = L"General";
    page.order    = 10;

    page.fields.push_back(SettingsFieldDescriptor{
        L"template.general.enabled",
        L"Enable plugin",
        L"Turns the template plugin behavior on or off.",
        SettingsFieldType::Bool, L"true", {}, 10
    });

    page.fields.push_back(SettingsFieldDescriptor{
        L"template.general.mode",
        L"Mode",
        L"Select the operating mode for your plugin.",
        SettingsFieldType::Enum, L"safe",
        {
            {L"safe", L"Safe"},
            {L"fast", L"Fast"},
            {L"custom", L"Custom"},
        },
        20
    });

    context.settingsRegistry->RegisterPage(std::move(page));
    return true;
}

void TemplatePlugin::Shutdown() {}