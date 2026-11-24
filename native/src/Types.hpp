#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace korektabar {

enum class DiagnosticCategory : uint8_t {
    Spelling = 0,     // Czerwone podkreślenie (ortografia / literówki)
    Punctuation = 1,  // Niebieskie podkreślenie (interpunkcja / gramatyka)
    Style = 2         // Żółte/bursztynowe podkreślenie (styl / powtórzenia / typografia)
};

struct DiagnosticSpan {
    uint32_t start = 0;       // Indeks w jednostkach UTF-16 (std::wstring)
    uint32_t length = 0;      // Długość w jednostkach UTF-16
    DiagnosticCategory category = DiagnosticCategory::Spelling;
    std::wstring title;       // Krótka etykieta do menu PPM
    std::wstring explanation; // Wyjaśnienie błędu
    std::vector<std::wstring> suggestions;
    bool autoFixable = false; // Czy bezpieczne dla "Auto-popraw wszystko"
};

struct FormatterOptions {
    bool collapseSpaces = true;
    bool fixPunctuationSpacing = true;
    bool normalizeParagraphs = true;
    bool capitalizeSentences = true;
};

struct AppConfig {
    uint32_t hotkeyModifiers = 0x0002; // MOD_CONTROL (Win32)
    uint32_t hotkeyVk = 0x20;          // VK_SPACE (Win32)
    bool enableSpelling = true;
    bool enablePunctuation = true;
    bool enableStyle = true;
    bool copyAndCloseAfterAutoFix = false;
    int32_t windowWidthDip = 880;
    int32_t windowHeightDip = 276;
    int32_t taskbarOffsetDip = 10;
    FormatterOptions formatter{};
};

struct UndoEntry {
    std::wstring text;
    uint32_t caret = 0;
    uint32_t anchor = 0;
};

#ifdef _WIN32
inline constexpr unsigned int WM_APP_DIAGNOSTICS_READY = 0x8001; // WM_APP + 1
inline constexpr unsigned int WM_APP_TRAY_ICON = 0x8002;         // WM_APP + 2
inline constexpr int HOTKEY_SUMMON_ID = 1;
#endif

} // namespace korektabar
