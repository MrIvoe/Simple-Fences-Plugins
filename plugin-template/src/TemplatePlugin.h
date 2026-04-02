#pragma once

#include "extensions/PluginContracts.h"

class TemplatePlugin final : public IPlugin
{
public:
    PluginManifest GetManifest() const override;
    bool Initialize(const PluginContext& context) override;
    void Shutdown() override;
};