#pragma once

#include <string>
#include <unordered_map>

namespace plugin_shared
{
    class ThemeTokenHelper
    {
    public:
        static bool ResolveToken(
            const std::unordered_map<std::string, std::string>& tokenValues,
            const std::unordered_map<std::string, std::string>& semanticMap,
            const std::string& semanticPath,
            std::string& outHexColor)
        {
            const auto semanticIt = semanticMap.find(semanticPath);
            if (semanticIt == semanticMap.end())
            {
                return false;
            }

            const auto tokenIt = tokenValues.find(semanticIt->second);
            if (tokenIt == tokenValues.end())
            {
                return false;
            }

            outHexColor = tokenIt->second;
            return true;
        }
    };
}
