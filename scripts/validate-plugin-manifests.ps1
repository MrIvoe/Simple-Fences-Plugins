$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$pluginFiles = Get-ChildItem (Join-Path $root 'plugins') -Recurse -Filter 'plugin.json'

$requiredKeys = @(
    'id',
    'displayName',
    'version',
    'description',
    'author',
    'enabledByDefault',
    'supportsSettingsPage',
    'supportsMainContentPage',
    'supportsHostedSummaryPanel',
    'icon',
    'updateChannelId',
    'hostedSummaryPanel',
    'capabilities',
    'repository'
)

$legacyCompatibilityKeys = @(
    'minHostVersion',
    'maxHostVersion',
    'minHostApiVersion',
    'maxHostApiVersion'
)

$semverPattern = '^\d+\.\d+\.\d+$'

$allowedPermissions = @(
    'read_space_metadata',
    'write_space_settings',
    'read_settings',
    'write_settings',
    'network_access',
    'filesystem_read',
    'filesystem_write',
    'execute_host_commands'
)

$failures = @()

foreach ($file in $pluginFiles)
{
    try
    {
        $json = Get-Content $file.FullName -Raw | ConvertFrom-Json
    }
    catch
    {
        $failures += "[$($file.FullName)] invalid JSON: $($_.Exception.Message)"
        continue
    }

    foreach ($key in $requiredKeys)
    {
        if (-not ($json.PSObject.Properties.Name -contains $key))
        {
            $failures += "[$($file.FullName)] missing required key: $key"
        }
    }

    if ($json.PSObject.Properties.Name -contains 'id' -and ($json.id -notmatch '^community\.[a-z0-9_]+$'))
    {
        $failures += "[$($file.FullName)] id must match community.<snake_case>"
    }

    if ($json.PSObject.Properties.Name -contains 'version' -and ($json.version -notmatch $semverPattern))
    {
        $failures += "[$($file.FullName)] version must be semantic (x.y.z)"
    }

    $hasCompatibilityObject = $json.PSObject.Properties.Name -contains 'compatibility'
    $hasLegacyCompatibility = $true
    foreach ($legacyKey in $legacyCompatibilityKeys)
    {
        if (-not ($json.PSObject.Properties.Name -contains $legacyKey))
        {
            $hasLegacyCompatibility = $false
            break
        }
    }

    if (-not $hasCompatibilityObject -and -not $hasLegacyCompatibility)
    {
        $failures += "[$($file.FullName)] must include either legacy host compatibility keys or a compatibility object"
    }

    if ($hasCompatibilityObject)
    {
        $compat = $json.compatibility
        if ($null -eq $compat)
        {
            $failures += "[$($file.FullName)] compatibility must not be null"
        }
        else
        {
            foreach ($scopeKey in @('hostVersion', 'hostApiVersion'))
            {
                if (-not ($compat.PSObject.Properties.Name -contains $scopeKey))
                {
                    $failures += "[$($file.FullName)] compatibility missing key: $scopeKey"
                    continue
                }

                $scope = $compat.$scopeKey
                if ($null -eq $scope)
                {
                    $failures += "[$($file.FullName)] compatibility.$scopeKey must not be null"
                    continue
                }

                foreach ($bound in @('min', 'max'))
                {
                    if (-not ($scope.PSObject.Properties.Name -contains $bound))
                    {
                        $failures += "[$($file.FullName)] compatibility.$scopeKey missing key: $bound"
                    }
                    elseif ([string]$scope.$bound -notmatch $semverPattern)
                    {
                        $failures += "[$($file.FullName)] compatibility.$scopeKey.$bound must be semantic (x.y.z)"
                    }
                }
            }
        }
    }

    if ($hasLegacyCompatibility)
    {
        foreach ($legacyKey in $legacyCompatibilityKeys)
        {
            if ([string]$json.$legacyKey -notmatch $semverPattern)
            {
                $failures += "[$($file.FullName)] $legacyKey must be semantic (x.y.z)"
            }
        }
    }

    if ($hasCompatibilityObject -and $hasLegacyCompatibility)
    {
        if ($json.minHostVersion -ne $json.compatibility.hostVersion.min -or
            $json.maxHostVersion -ne $json.compatibility.hostVersion.max -or
            $json.minHostApiVersion -ne $json.compatibility.hostApiVersion.min -or
            $json.maxHostApiVersion -ne $json.compatibility.hostApiVersion.max)
        {
            $failures += "[$($file.FullName)] legacy compatibility keys must match compatibility object values when both are present"
        }
    }

    if ($json.PSObject.Properties.Name -contains 'permissions')
    {
        $permissions = @($json.permissions)
        foreach ($permission in $permissions)
        {
            if ($permission -notin $allowedPermissions)
            {
                $failures += "[$($file.FullName)] unsupported permission: $permission"
            }
        }
    }

    if ($json.PSObject.Properties.Name -contains 'updateChannelId')
    {
        if ($json.updateChannelId -notin @('stable', 'preview'))
        {
            $failures += "[$($file.FullName)] updateChannelId must be stable or preview"
        }
    }

    if ($json.PSObject.Properties.Name -contains 'capabilities')
    {
        $caps = @($json.capabilities)
        $hasSettings = $caps -contains 'settings_pages'
        $hasHostedContent = ($caps -contains 'space_content_provider') -or ($caps -contains 'widgets')

        if ($json.PSObject.Properties.Name -contains 'supportsSettingsPage' -and [bool]$json.supportsSettingsPage -ne [bool]$hasSettings)
        {
            $failures += "[$($file.FullName)] supportsSettingsPage does not match capabilities"
        }

        if ($json.PSObject.Properties.Name -contains 'supportsMainContentPage' -and [bool]$json.supportsMainContentPage -ne [bool]$hasHostedContent)
        {
            $failures += "[$($file.FullName)] supportsMainContentPage does not match capabilities"
        }

        if ($json.PSObject.Properties.Name -contains 'supportsHostedSummaryPanel')
        {
            if (-not [bool]$json.supportsHostedSummaryPanel)
            {
                $failures += "[$($file.FullName)] supportsHostedSummaryPanel must be true so every plugin has a host-rendered panel surface"
            }
        }

        if ($json.PSObject.Properties.Name -contains 'hostedSummaryPanel')
        {
            $panel = $json.hostedSummaryPanel
            if ($null -eq $panel)
            {
                $failures += "[$($file.FullName)] hostedSummaryPanel must be present"
            }
            else
            {
                foreach ($panelKey in @('panelId', 'title', 'schemaVersion', 'layout', 'themeTokenNamespace', 'sections'))
                {
                    if (-not ($panel.PSObject.Properties.Name -contains $panelKey))
                    {
                        $failures += "[$($file.FullName)] hostedSummaryPanel missing key: $panelKey"
                    }
                }

                if ($panel.PSObject.Properties.Name -contains 'schemaVersion' -and $panel.schemaVersion -ne '1')
                {
                    $failures += "[$($file.FullName)] hostedSummaryPanel.schemaVersion must be '1'"
                }

                if ($panel.PSObject.Properties.Name -contains 'themeTokenNamespace' -and $panel.themeTokenNamespace -ne 'win32_theme_system')
                {
                    $failures += "[$($file.FullName)] hostedSummaryPanel.themeTokenNamespace must be win32_theme_system"
                }

                if ($panel.PSObject.Properties.Name -contains 'sections')
                {
                    $sections = @($panel.sections)
                    if ($sections.Count -eq 0)
                    {
                        $failures += "[$($file.FullName)] hostedSummaryPanel.sections must contain at least one section"
                    }
                    foreach ($section in $sections)
                    {
                        foreach ($sectionKey in @('id', 'title', 'description', 'iconToken', 'surfaceToken'))
                        {
                            if (-not ($section.PSObject.Properties.Name -contains $sectionKey))
                            {
                                $failures += "[$($file.FullName)] hostedSummaryPanel section missing key: $sectionKey"
                            }
                        }
                    }
                }
            }
        }
    }
}

if ($failures.Count -gt 0)
{
    Write-Host 'Manifest validation failed:' -ForegroundColor Red
    $failures | ForEach-Object { Write-Host " - $_" -ForegroundColor Red }
    exit 1
}

Write-Host "Validated $($pluginFiles.Count) plugin manifest files successfully." -ForegroundColor Green
