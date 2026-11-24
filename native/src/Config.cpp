#include "Config.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>

#include <filesystem>

namespace korektabar {

namespace {
int ReadIniInt(const wchar_t* section, const wchar_t* key, int defaultVal, const wchar_t* path) {
    return static_cast<int>(GetPrivateProfileIntW(section, key, defaultVal, path));
}

void WriteIniInt(const wchar_t* section, const wchar_t* key, int val, const wchar_t* path) {
    wchar_t buf[32]{};
    swprintf(buf, 32, L"%d", val);
    WritePrivateProfileStringW(section, key, buf, path);
}
} // namespace

std::wstring ConfigManager::GetConfigFilePath() {
    PWSTR appData = nullptr;
    std::wstring baseDir = L".";
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appData)) && appData) {
        baseDir = appData;
        CoTaskMemFree(appData);
    }
    std::filesystem::path dir = std::filesystem::path(baseDir) / L"KorektaBar";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return (dir / L"config.ini").wstring();
}

AppConfig ConfigManager::Load() {
    AppConfig cfg;
    const std::wstring path = GetConfigFilePath();
    const wchar_t* p = path.c_str();

    cfg.hotkeyModifiers = static_cast<uint32_t>(ReadIniInt(L"Hotkey", L"Modifiers", MOD_CONTROL, p));
    cfg.hotkeyVk = static_cast<uint32_t>(ReadIniInt(L"Hotkey", L"VirtualKey", VK_SPACE, p));
    if (cfg.hotkeyVk == 0) cfg.hotkeyVk = VK_SPACE;

    cfg.enableSpelling = ReadIniInt(L"Analyzer", L"Spelling", 1, p) != 0;
    cfg.enablePunctuation = ReadIniInt(L"Analyzer", L"Punctuation", 1, p) != 0;
    cfg.enableStyle = ReadIniInt(L"Analyzer", L"Style", 1, p) != 0;
    cfg.copyAndCloseAfterAutoFix = ReadIniInt(L"Behavior", L"CopyAndClose", 0, p) != 0;

    cfg.windowWidthDip = ReadIniInt(L"Window", L"WidthDip", 880, p);
    cfg.windowHeightDip = ReadIniInt(L"Window", L"HeightDip", 276, p);
    cfg.taskbarOffsetDip = ReadIniInt(L"Window", L"TaskbarOffsetDip", 10, p);

    cfg.formatter.collapseSpaces = ReadIniInt(L"Formatter", L"CollapseSpaces", 1, p) != 0;
    cfg.formatter.fixPunctuationSpacing = ReadIniInt(L"Formatter", L"FixPunctuationSpacing", 1, p) != 0;
    cfg.formatter.normalizeParagraphs = ReadIniInt(L"Formatter", L"NormalizeParagraphs", 1, p) != 0;
    cfg.formatter.capitalizeSentences = ReadIniInt(L"Formatter", L"CapitalizeSentences", 1, p) != 0;

    return cfg;
}

bool ConfigManager::Save(const AppConfig& cfg) {
    const std::wstring path = GetConfigFilePath();
    const wchar_t* p = path.c_str();

    WriteIniInt(L"Hotkey", L"Modifiers", static_cast<int>(cfg.hotkeyModifiers), p);
    WriteIniInt(L"Hotkey", L"VirtualKey", static_cast<int>(cfg.hotkeyVk), p);

    WriteIniInt(L"Analyzer", L"Spelling", cfg.enableSpelling ? 1 : 0, p);
    WriteIniInt(L"Analyzer", L"Punctuation", cfg.enablePunctuation ? 1 : 0, p);
    WriteIniInt(L"Analyzer", L"Style", cfg.enableStyle ? 1 : 0, p);
    WriteIniInt(L"Behavior", L"CopyAndClose", cfg.copyAndCloseAfterAutoFix ? 1 : 0, p);

    WriteIniInt(L"Window", L"WidthDip", cfg.windowWidthDip, p);
    WriteIniInt(L"Window", L"HeightDip", cfg.windowHeightDip, p);
    WriteIniInt(L"Window", L"TaskbarOffsetDip", cfg.taskbarOffsetDip, p);

    WriteIniInt(L"Formatter", L"CollapseSpaces", cfg.formatter.collapseSpaces ? 1 : 0, p);
    WriteIniInt(L"Formatter", L"FixPunctuationSpacing", cfg.formatter.fixPunctuationSpacing ? 1 : 0, p);
    WriteIniInt(L"Formatter", L"NormalizeParagraphs", cfg.formatter.normalizeParagraphs ? 1 : 0, p);
    WriteIniInt(L"Formatter", L"CapitalizeSentences", cfg.formatter.capitalizeSentences ? 1 : 0, p);

    return true;
}

std::wstring ConfigManager::FormatHotkey(uint32_t modifiers, uint32_t vk) {
    std::wstring result;
    if (modifiers & MOD_CONTROL) result += L"Ctrl + ";
    if (modifiers & MOD_ALT) result += L"Alt + ";
    if (modifiers & MOD_SHIFT) result += L"Shift + ";
    if (modifiers & MOD_WIN) result += L"Win + ";

    if (vk == VK_SPACE) {
        result += L"Space";
    } else if (vk >= 'A' && vk <= 'Z') {
        result.push_back(static_cast<wchar_t>(vk));
    } else if (vk >= '0' && vk <= '9') {
        result.push_back(static_cast<wchar_t>(vk));
    } else if (vk >= VK_F1 && vk <= VK_F12) {
        result += L"F" + std::to_wstring(vk - VK_F1 + 1);
    } else if (vk == VK_OEM_3) {
        result += L"`";
    } else if (vk == VK_TAB) {
        result += L"Tab";
    } else if (vk == VK_RETURN) {
        result += L"Enter";
    } else {
        UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        wchar_t keyName[64]{};
        if (GetKeyNameTextW(static_cast<LONG>(scanCode << 16), keyName, 64) > 0) {
            result += keyName;
        } else {
            result += L"Key(" + std::to_wstring(vk) + L")";
        }
    }
    return result;
}

} // namespace korektabar
