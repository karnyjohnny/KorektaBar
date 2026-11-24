#pragma once

#include "Types.hpp"
#include <string>

namespace korektabar {

class ConfigManager {
public:
    static std::wstring GetConfigFilePath();
    static AppConfig Load();
    static bool Save(const AppConfig& cfg);
    static std::wstring FormatHotkey(uint32_t modifiers, uint32_t vk);
};

} // namespace korektabar
