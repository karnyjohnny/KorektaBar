#include "MainWindow.hpp"
#include "Config.hpp"
#include "SettingsDialog.hpp"
#include "TextFormatter.hpp"
#include "../resources/resource.h"

#include <dwmapi.h>
#include <algorithm>
#include <cwctype>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace korektabar {

namespace {

constexpr int BTN_AUTOFIX = 101;
constexpr int BTN_FORMAT = 102;
constexpr int BTN_COPY = 103;
constexpr int BTN_UNDO = 104;
constexpr int BTN_SETTINGS = 105;
constexpr int BTN_HIDE = 106;

constexpr UINT_PTR TIMER_CARET_BLINK = 1;
constexpr UINT_PTR TIMER_TRIM_MEMORY = 2;

const IID kIidDWriteFactoryMain = {
    0xb859ee5a, 0xd838, 0x4b5b, {0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48}
};

D2D1_COLOR_F Rgb(uint32_t hex, float a = 1.0f) {
    return D2D1::ColorF(
        static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
        static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
        static_cast<float>(hex & 0xFF) / 255.0f,
        a);
}

float GetWindowDpiSafe(HWND hwnd) {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
        auto fn = reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"));
        if (fn && hwnd) {
            UINT d = fn(hwnd);
            if (d > 0) return static_cast<float>(d);
        }
    }
    HDC dc = GetDC(hwnd);
    float d = dc ? static_cast<float>(GetDeviceCaps(dc, LOGPIXELSX)) : 96.0f;
    if (dc) ReleaseDC(hwnd, dc);
    return d;
}

std::wstring ToLowerPolish(const std::wstring& s) {
    std::wstring out;
    out.reserve(s.size());
    for (wchar_t ch : s) {
        if (ch >= L'A' && ch <= L'Z') out.push_back(static_cast<wchar_t>(ch - L'A' + L'a'));
        else if (ch == L'Ą') out.push_back(L'ą');
        else if (ch == L'Ć') out.push_back(L'ć');
        else if (ch == L'Ę') out.push_back(L'ę');
        else if (ch == L'Ł') out.push_back(L'ł');
        else if (ch == L'Ń') out.push_back(L'ń');
        else if (ch == L'Ó') out.push_back(L'ó');
        else if (ch == L'Ś') out.push_back(L'ś');
        else if (ch == L'Ź') out.push_back(L'ź');
        else if (ch == L'Ż') out.push_back(L'ż');
        else out.push_back(ch);
    }
    return out;
}

} // namespace

MainWindow::~MainWindow() {
    worker_.Stop();
    RemoveTrayIcon();
    if (hwnd_) {
        UnregisterHotKey(hwnd_, HOTKEY_SUMMON_ID);
    }
    ReleaseGraphicsResources();
    if (uiFont_) { uiFont_->Release(); uiFont_ = nullptr; }
    if (uiBoldFont_) { uiBoldFont_->Release(); uiBoldFont_ = nullptr; }
    if (statusFont_) { statusFont_->Release(); statusFont_ = nullptr; }
    if (dwriteFactory_) { dwriteFactory_->Release(); dwriteFactory_ = nullptr; }
    if (d2dFactory_) { d2dFactory_->Release(); d2dFactory_ = nullptr; }
}

bool MainWindow::Create(HINSTANCE hInstance) {
    hInstance_ = hInstance;
    config_ = ConfigManager::Load();

    const wchar_t* kClassName = L"KorektaBarMainWindowClass";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = &MainWindow::WndProc;
    wc.hInstance = hInstance_;
    wc.hIcon = LoadIconW(hInstance_, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;

    if (!RegisterClassExW(&wc)) return false;

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory_);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, kIidDWriteFactoryMain, reinterpret_cast<IUnknown**>(&dwriteFactory_));
    if (dwriteFactory_) {
        dwriteFactory_->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL, 12.5f, L"pl-PL", &uiFont_);
        dwriteFactory_->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL, 12.5f, L"pl-PL", &uiBoldFont_);
        dwriteFactory_->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL, 11.5f, L"pl-PL", &statusFont_);
        if (uiFont_) {
            uiFont_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            uiFont_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
        if (uiBoldFont_) {
            uiBoldFont_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            uiBoldFont_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    // Okno typu Tool/Popup o proporcjach klawiatury ekranowej, bez ciężkiej ramki systemowej
    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        kClassName,
        L"KorektaBar",
        WS_POPUP | WS_THICKFRAME | WS_CLIPCHILDREN,
        0, 0, 880, 276,
        nullptr,
        nullptr,
        hInstance_,
        this);

    if (!hwnd_) return false;

    dpi_ = GetWindowDpiSafe(hwnd_);
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    DWORD corner = 2; // DWMWCP_ROUND (Windows 11+ zaokrąglone rogi, bezpiecznie ignorowane na Win 10)
    DwmSetWindowAttribute(hwnd_, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corner, sizeof(corner));

    editor_.Initialize(hwnd_, dpi_);
    editor_.onTextChanged = [this]() { TriggerBackgroundAnalysis(); };
    editor_.onRequestRepaint = [this]() {
        if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
    };

    worker_.Start(hwnd_);
    AddTrayIcon();
    RegisterConfiguredHotkey();
    RepositionAboveTaskbar();

    SetTimer(hwnd_, TIMER_CARET_BLINK, 530, nullptr);
    return true;
}

void MainWindow::RepositionAboveTaskbar() {
    if (!hwnd_) return;
    dpi_ = GetWindowDpiSafe(hwnd_);
    float scale = dpi_ / 96.0f;

    POINT cursor{};
    GetCursorPos(&cursor);
    HMONITOR mon = MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(mon, &mi);

    int workW = mi.rcWork.right - mi.rcWork.left;
    int workH = mi.rcWork.bottom - mi.rcWork.top;

    int winW = std::min(workW - static_cast<int>(24 * scale), static_cast<int>(config_.windowWidthDip * scale));
    int winH = std::min(workH - static_cast<int>(40 * scale), static_cast<int>(config_.windowHeightDip * scale));
    int gap = static_cast<int>(config_.taskbarOffsetDip * scale);

    int x = mi.rcWork.left + (workW - winW) / 2;
    int y = mi.rcWork.bottom - winH - gap;

    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, winW, winH, SWP_NOACTIVATE);

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    LayoutControls(static_cast<float>(rc.right - rc.left) / scale,
                   static_cast<float>(rc.bottom - rc.top) / scale);
}

void MainWindow::LayoutControls(float w, float h) {
    buttons_.clear();
    const float topBarY = 10.0f;
    const float btnH = 28.0f;

    float rightX = w - 14.0f;
    auto pushRight = [&](int id, const wchar_t* label, float bw, bool primary = false) {
        rightX -= bw;
        buttons_.push_back({id, label, rightX, topBarY, bw, btnH, primary});
        rightX -= 6.0f;
    };

    pushRight(BTN_HIDE, L"Ukryj", 56.0f, false);
    pushRight(BTN_SETTINGS, L"Opcje", 62.0f, false);
    pushRight(BTN_UNDO, L"Cofnij", 64.0f, false);
    pushRight(BTN_COPY, L"Kopiuj", 68.0f, false);
    pushRight(BTN_FORMAT, L"Formatuj tekst", 112.0f, false);
    pushRight(BTN_AUTOFIX, L"Auto-popraw wszystko", 156.0f, true);

    editor_.SetBoundsDip(14.0f, 48.0f, std::max(200.0f, w - 28.0f), std::max(80.0f, h - 82.0f));
}

void MainWindow::ShowAndFocus() {
    if (!hwnd_) return;
    KillTimer(hwnd_, TIMER_TRIM_MEMORY);
    RepositionAboveTaskbar();
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
    SetFocus(hwnd_);
    editor_.SetFocused(true);
    visible_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::HideToTray() {
    if (!hwnd_) return;
    ShowWindow(hwnd_, SW_HIDE);
    visible_ = false;
    editor_.SetFocused(false);
    // Po 3 sekundach w tle przytnij nieużywane strony robocze RAM
    SetTimer(hwnd_, TIMER_TRIM_MEMORY, 3000, nullptr);
}

void MainWindow::ToggleVisibility() {
    if (visible_ && IsWindowVisible(hwnd_)) {
        HideToTray();
    } else {
        ShowAndFocus();
    }
}

void MainWindow::TrimIdleMemory() {
    ReleaseGraphicsResources();
    SetProcessWorkingSetSizeEx(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1), 0);
}

bool MainWindow::EnsureGraphicsResources() {
    if (renderTarget_) return true;
    if (!d2dFactory_ || !hwnd_) return false;

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<UINT>(std::max(1L, rc.right - rc.left)),
        static_cast<UINT>(std::max(1L, rc.bottom - rc.top)));

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    props.dpiX = dpi_;
    props.dpiY = dpi_;

    HRESULT hr = d2dFactory_->CreateHwndRenderTarget(
        props,
        D2D1::HwndRenderTargetProperties(hwnd_, size),
        &renderTarget_);
    return SUCCEEDED(hr) && renderTarget_ != nullptr;
}

void MainWindow::ReleaseGraphicsResources() {
    if (renderTarget_) {
        renderTarget_->Release();
        renderTarget_ = nullptr;
    }
}

void MainWindow::Render() {
    if (!EnsureGraphicsResources()) return;

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    float scale = dpi_ / 96.0f;
    float w = static_cast<float>(rc.right - rc.left) / scale;
    float h = static_cast<float>(rc.bottom - rc.top) / scale;

    renderTarget_->BeginDraw();
    renderTarget_->Clear(Rgb(0x14161A));

    ID2D1SolidColorBrush* borderBrush = nullptr;
    ID2D1SolidColorBrush* titleBrush = nullptr;
    ID2D1SolidColorBrush* mutedBrush = nullptr;
    ID2D1SolidColorBrush* btnBg = nullptr;
    ID2D1SolidColorBrush* btnHover = nullptr;
    ID2D1SolidColorBrush* primaryBg = nullptr;
    ID2D1SolidColorBrush* primaryHover = nullptr;
    ID2D1SolidColorBrush* whiteBrush = nullptr;
    ID2D1SolidColorBrush* spellDot = nullptr;
    ID2D1SolidColorBrush* punctDot = nullptr;
    ID2D1SolidColorBrush* styleDot = nullptr;

    renderTarget_->CreateSolidColorBrush(Rgb(0x2A2F3A), &borderBrush);
    renderTarget_->CreateSolidColorBrush(Rgb(0xECEFF4), &titleBrush);
    renderTarget_->CreateSolidColorBrush(Rgb(0x94A3B8), &mutedBrush);
    renderTarget_->CreateSolidColorBrush(Rgb(0x1F242D), &btnBg);
    renderTarget_->CreateSolidColorBrush(Rgb(0x2A303C), &btnHover);
    renderTarget_->CreateSolidColorBrush(Rgb(0x2563EB), &primaryBg);
    renderTarget_->CreateSolidColorBrush(Rgb(0x3B82F6), &primaryHover);
    renderTarget_->CreateSolidColorBrush(Rgb(0xFFFFFF), &whiteBrush);
    renderTarget_->CreateSolidColorBrush(Rgb(0xF87171), &spellDot);
    renderTarget_->CreateSolidColorBrush(Rgb(0x38BDF8), &punctDot);
    renderTarget_->CreateSolidColorBrush(Rgb(0xFBBF24), &styleDot);

    // Zewnętrzne obramowanie 1px
    renderTarget_->DrawRectangle(D2D1::RectF(0.5f, 0.5f, w - 0.5f, h - 0.5f), borderBrush, 1.0f);

    // Tytuł i skrót po lewej stronie paska narzędzi (uchwyt przeciągania)
    if (uiBoldFont_ && titleBrush) {
        D2D1_TEXT_ALIGNMENT prevAlign = uiBoldFont_->GetTextAlignment();
        uiBoldFont_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        renderTarget_->DrawTextW(L"KorektaBar", 10, uiBoldFont_,
                                 D2D1::RectF(16.0f, 10.0f, 130.0f, 38.0f), titleBrush);
        uiBoldFont_->SetTextAlignment(prevAlign);
    }
    if (statusFont_ && mutedBrush) {
        std::wstring hk = ConfigManager::FormatHotkey(config_.hotkeyModifiers, config_.hotkeyVk);
        renderTarget_->DrawTextW(hk.c_str(), static_cast<UINT32>(hk.size()), statusFont_,
                                 D2D1::RectF(108.0f, 16.0f, 250.0f, 36.0f), mutedBrush);
    }

    // Przyciski paska narzędzi
    for (const auto& b : buttons_) {
        bool hover = (b.id == hoveredButtonId_);
        ID2D1SolidColorBrush* fill = b.primary ? (hover ? primaryHover : primaryBg) : (hover ? btnHover : btnBg);
        D2D1_ROUNDED_RECT r = D2D1::RoundedRect(D2D1::RectF(b.x, b.y, b.x + b.w, b.y + b.h), 6.0f, 6.0f);
        renderTarget_->FillRoundedRectangle(r, fill);
        if (!b.primary) {
            renderTarget_->DrawRoundedRectangle(r, borderBrush, 1.0f);
        }
        IDWriteTextFormat* fmt = b.primary ? uiBoldFont_ : uiFont_;
        if (fmt) {
            renderTarget_->DrawTextW(b.label.c_str(), static_cast<UINT32>(b.label.size()),
                                     fmt, D2D1::RectF(b.x, b.y, b.x + b.w, b.y + b.h),
                                     b.primary ? whiteBrush : titleBrush);
        }
    }

    // Główny obszar edycji Direct2D/DirectWrite
    editor_.Render(renderTarget_);

    // Dolny pasek podsumowania błędów (z jawnymi etykietami — bez samego kodowania kolorem)
    int spellCnt = 0, punctCnt = 0, styleCnt = 0;
    for (const auto& sp : editor_.GetDiagnostics()) {
        if (sp.category == DiagnosticCategory::Spelling) ++spellCnt;
        else if (sp.category == DiagnosticCategory::Punctuation) ++punctCnt;
        else ++styleCnt;
    }

    float footerY = h - 25.0f;
    float cx = 16.0f;
    auto drawLegend = [&](ID2D1SolidColorBrush* dot, const std::wstring& txt) {
        renderTarget_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx + 4.0f, footerY + 7.5f), 3.5f, 3.5f), dot);
        if (statusFont_ && mutedBrush) {
            renderTarget_->DrawTextW(txt.c_str(), static_cast<UINT32>(txt.size()), statusFont_,
                                     D2D1::RectF(cx + 12.0f, footerY, cx + 150.0f, footerY + 18.0f), mutedBrush);
        }
        cx += 132.0f;
    };

    drawLegend(spellDot, L"Ortografia: " + std::to_wstring(spellCnt));
    drawLegend(punctDot, L"Interpunkcja: " + std::to_wstring(punctCnt));
    drawLegend(styleDot, L"Styl: " + std::to_wstring(styleCnt));

    if (statusFont_ && mutedBrush) {
        std::wstring info = L"PPM na słowie: poprawki  ·  Znaków: " + std::to_wstring(editor_.GetText().size());
        D2D1_TEXT_ALIGNMENT prev = statusFont_->GetTextAlignment();
        statusFont_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        renderTarget_->DrawTextW(info.c_str(), static_cast<UINT32>(info.size()), statusFont_,
                                 D2D1::RectF(w - 340.0f, footerY, w - 16.0f, footerY + 18.0f), mutedBrush);
        statusFont_->SetTextAlignment(prev);
    }

    if (borderBrush) borderBrush->Release();
    if (titleBrush) titleBrush->Release();
    if (mutedBrush) mutedBrush->Release();
    if (btnBg) btnBg->Release();
    if (btnHover) btnHover->Release();
    if (primaryBg) primaryBg->Release();
    if (primaryHover) primaryHover->Release();
    if (whiteBrush) whiteBrush->Release();
    if (spellDot) spellDot->Release();
    if (punctDot) punctDot->Release();
    if (styleDot) styleDot->Release();

    HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        ReleaseGraphicsResources();
    }
}

void MainWindow::TriggerBackgroundAnalysis() {
    AnalysisOptions opts;
    opts.checkSpelling = config_.enableSpelling;
    opts.checkPunctuation = config_.enablePunctuation;
    opts.checkStyle = config_.enableStyle;
    worker_.ScheduleAnalysis(editor_.GetText(), opts, ignoredWords_);
}

void MainWindow::ApplyFastFormatter() {
    std::wstring formatted = FormatPolishText(editor_.GetText(), config_.formatter);
    editor_.SetText(formatted, true);
}

void MainWindow::RunAutoFixAll() {
    int count = editor_.AutoFixAll();
    if (count > 0 && config_.copyAndCloseAfterAutoFix) {
        CopyAndHide();
    }
}

void MainWindow::CopyAndHide() {
    editor_.SelectAll();
    editor_.CopyToClipboard();
    HideToTray();
}

void MainWindow::OpenSettings() {
    if (ShowSettingsDialog(hwnd_, config_)) {
        RegisterConfiguredHotkey();
        TriggerBackgroundAnalysis();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void MainWindow::ShowEditorContextMenu(float dipX, float dipY, int screenX, int screenY) {
    const DiagnosticSpan* hit = editor_.HitTestDiagnostic(dipX, dipY);
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    constexpr UINT CMD_SUGGEST_BASE = 5000;
    constexpr UINT CMD_IGNORE_WORD = 5100;
    constexpr UINT CMD_COPY = 5201;
    constexpr UINT CMD_PASTE = 5202;
    constexpr UINT CMD_SELECT_ALL = 5203;
    constexpr UINT CMD_AUTOFIX_ALL = 5204;

    DiagnosticSpan capturedSpan;
    bool hasHit = (hit != nullptr);
    if (hit) {
        capturedSpan = *hit;
        std::wstring header = hit->title + L" — " + hit->explanation;
        AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, header.c_str());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

        if (hit->suggestions.empty()) {
            AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, L"(Brak automatycznej propozycji)");
        } else {
            for (size_t i = 0; i < hit->suggestions.size() && i < 6; ++i) {
                std::wstring label = hit->suggestions[i].empty()
                                         ? L"Usuń zbędny znak"
                                         : (L"Zamień na: „" + hit->suggestions[i] + L"”");
                AppendMenuW(menu, MF_STRING, CMD_SUGGEST_BASE + static_cast<UINT>(i), label.c_str());
            }
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, CMD_IGNORE_WORD, L"Ignoruj w tej sesji");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    }

    AppendMenuW(menu, MF_STRING, CMD_AUTOFIX_ALL, L"Auto-popraw wszystko");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, CMD_COPY, L"Kopiuj (Ctrl+C)");
    AppendMenuW(menu, MF_STRING, CMD_PASTE, L"Wklej (Ctrl+V)");
    AppendMenuW(menu, MF_STRING, CMD_SELECT_ALL, L"Zaznacz wszystko (Ctrl+A)");

    UINT cmd = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screenX, screenY, hwnd_, nullptr);
    DestroyMenu(menu);

    if (hasHit && cmd >= CMD_SUGGEST_BASE && cmd < CMD_SUGGEST_BASE + 10) {
        size_t idx = cmd - CMD_SUGGEST_BASE;
        if (idx < capturedSpan.suggestions.size()) {
            editor_.ApplyDiagnosticFix(capturedSpan, capturedSpan.suggestions[idx]);
        }
    } else if (hasHit && cmd == CMD_IGNORE_WORD) {
        if (capturedSpan.start + capturedSpan.length <= editor_.GetText().size()) {
            std::wstring word = editor_.GetText().substr(capturedSpan.start, capturedSpan.length);
            ignoredWords_.insert(ToLowerPolish(word));
            TriggerBackgroundAnalysis();
        }
    } else if (cmd == CMD_AUTOFIX_ALL) {
        RunAutoFixAll();
    } else if (cmd == CMD_COPY) {
        editor_.CopyToClipboard();
    } else if (cmd == CMD_PASTE) {
        editor_.PasteFromClipboard();
    } else if (cmd == CMD_SELECT_ALL) {
        editor_.SelectAll();
    }
}

void MainWindow::ShowTrayMenu() {
    POINT pt{};
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    std::wstring hk = ConfigManager::FormatHotkey(config_.hotkeyModifiers, config_.hotkeyVk);
    std::wstring toggleLabel = (visible_ ? L"Ukryj KorektaBar (" : L"Pokaż KorektaBar (") + hk + L")";

    AppendMenuW(menu, MF_STRING, 6001, toggleLabel.c_str());
    AppendMenuW(menu, MF_STRING, 6002, L"Ustawienia…");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 6003, L"Zamknij");

    SetForegroundWindow(hwnd_);
    UINT cmd = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, hwnd_, nullptr);
    DestroyMenu(menu);

    if (cmd == 6001) ToggleVisibility();
    else if (cmd == 6002) {
        if (!visible_) ShowAndFocus();
        OpenSettings();
    } else if (cmd == 6003) {
        DestroyWindow(hwnd_);
    }
}

void MainWindow::RegisterConfiguredHotkey() {
    if (!hwnd_) return;
    UnregisterHotKey(hwnd_, HOTKEY_SUMMON_ID);
    RegisterHotKey(hwnd_, HOTKEY_SUMMON_ID, config_.hotkeyModifiers | MOD_NOREPEAT, config_.hotkeyVk);
}

void MainWindow::AddTrayIcon() {
    if (trayAdded_ || !hwnd_) return;
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = hwnd_;
    nid_.uID = 1;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = WM_APP_TRAY_ICON;
    nid_.hIcon = LoadIconW(hInstance_, MAKEINTRESOURCEW(IDI_APP_ICON));
    wcsncpy(nid_.szTip, L"KorektaBar — Korektor PL (Ctrl+Space)", 127);
    nid_.szTip[127] = L'\0';
    Shell_NotifyIconW(NIM_ADD, &nid_);
    trayAdded_ = true;
}

void MainWindow::RemoveTrayIcon() {
    if (!trayAdded_) return;
    Shell_NotifyIconW(NIM_DELETE, &nid_);
    trayAdded_ = false;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) return self->HandleMessage(msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    float scale = dpi_ / 96.0f;

    switch (msg) {
        case WM_NCCALCSIZE:
            if (wParam == TRUE) return 0; // Czyste nowoczesne okno bez grubego paska systemowego
            break;

        case WM_NCHITTEST: {
            POINT pt{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
            ScreenToClient(hwnd_, &pt);
            RECT rc{};
            GetClientRect(hwnd_, &rc);
            constexpr int border = 6;
            if (pt.x < border && pt.y < border) return HTTOPLEFT;
            if (pt.x > rc.right - border && pt.y < border) return HTTOPRIGHT;
            if (pt.x < border && pt.y > rc.bottom - border) return HTBOTTOMLEFT;
            if (pt.x > rc.right - border && pt.y > rc.bottom - border) return HTBOTTOMRIGHT;
            if (pt.x < border) return HTLEFT;
            if (pt.x > rc.right - border) return HTRIGHT;
            if (pt.y < border) return HTTOP;
            if (pt.y > rc.bottom - border) return HTBOTTOM;

            float dipX = static_cast<float>(pt.x) / scale;
            float dipY = static_cast<float>(pt.y) / scale;
            if (dipY < 42.0f) {
                bool onBtn = false;
                for (const auto& b : buttons_) {
                    if (dipX >= b.x && dipX <= b.x + b.w && dipY >= b.y && dipY <= b.y + b.h) {
                        onBtn = true;
                        break;
                    }
                }
                if (!onBtn) return HTCAPTION;
            }
            return HTCLIENT;
        }

        case WM_SIZE: {
            UINT pw = LOWORD(lParam);
            UINT ph = HIWORD(lParam);
            if (renderTarget_) {
                renderTarget_->Resize(D2D1::SizeU(pw, ph));
            }
            LayoutControls(static_cast<float>(pw) / scale, static_cast<float>(ph) / scale);
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        case WM_DPICHANGED: {
            dpi_ = static_cast<float>(HIWORD(wParam));
            editor_.SetDpi(dpi_);
            if (renderTarget_) renderTarget_->SetDpi(dpi_, dpi_);
            auto* prc = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(hwnd_, nullptr, prc->left, prc->top,
                         prc->right - prc->left, prc->bottom - prc->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps{};
            BeginPaint(hwnd_, &ps);
            Render();
            EndPaint(hwnd_, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_HOTKEY:
            if (wParam == HOTKEY_SUMMON_ID) {
                ToggleVisibility();
            }
            return 0;

        case WM_APP_TRAY_ICON:
            if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
                ToggleVisibility();
            } else if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                ShowTrayMenu();
            }
            return 0;

        case WM_APP_DIAGNOSTICS_READY: {
            uint64_t rev = static_cast<uint64_t>(wParam);
            std::vector<DiagnosticSpan> spans;
            if (worker_.ConsumeResults(rev, spans)) {
                editor_.SetDiagnostics(std::move(spans));
            }
            return 0;
        }

        case WM_TIMER:
            if (wParam == TIMER_CARET_BLINK && visible_) {
                editor_.ToggleCaretBlink();
            } else if (wParam == TIMER_TRIM_MEMORY) {
                KillTimer(hwnd_, TIMER_TRIM_MEMORY);
                if (!visible_) TrimIdleMemory();
            }
            return 0;

        case WM_MOUSEMOVE: {
            float dipX = static_cast<float>(static_cast<short>(LOWORD(lParam))) / scale;
            float dipY = static_cast<float>(static_cast<short>(HIWORD(lParam))) / scale;
            int hitBtn = 0;
            for (const auto& b : buttons_) {
                if (dipX >= b.x && dipX <= b.x + b.w && dipY >= b.y && dipY <= b.y + b.h) {
                    hitBtn = b.id;
                    break;
                }
            }
            if (hitBtn != hoveredButtonId_) {
                hoveredButtonId_ = hitBtn;
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            editor_.OnMouseMove(dipX, dipY);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            SetCapture(hwnd_);
            SetFocus(hwnd_);
            float dipX = static_cast<float>(static_cast<short>(LOWORD(lParam))) / scale;
            float dipY = static_cast<float>(static_cast<short>(HIWORD(lParam))) / scale;

            for (const auto& b : buttons_) {
                if (dipX >= b.x && dipX <= b.x + b.w && dipY >= b.y && dipY <= b.y + b.h) {
                    ReleaseCapture();
                    if (b.id == BTN_AUTOFIX) RunAutoFixAll();
                    else if (b.id == BTN_FORMAT) ApplyFastFormatter();
                    else if (b.id == BTN_COPY) CopyAndHide();
                    else if (b.id == BTN_UNDO) editor_.Undo();
                    else if (b.id == BTN_SETTINGS) OpenSettings();
                    else if (b.id == BTN_HIDE) HideToTray();
                    return 0;
                }
            }

            if (editor_.ContainsPoint(dipX, dipY)) {
                editor_.OnMouseDown(dipX, dipY, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
            }
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            float dipX = static_cast<float>(static_cast<short>(LOWORD(lParam))) / scale;
            float dipY = static_cast<float>(static_cast<short>(HIWORD(lParam))) / scale;
            if (editor_.ContainsPoint(dipX, dipY)) {
                editor_.OnMouseDoubleClick(dipX, dipY);
            }
            return 0;
        }

        case WM_LBUTTONUP:
            ReleaseCapture();
            editor_.OnMouseUp();
            return 0;

        case WM_RBUTTONUP: {
            POINT pt{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
            float dipX = static_cast<float>(pt.x) / scale;
            float dipY = static_cast<float>(pt.y) / scale;
            ClientToScreen(hwnd_, &pt);
            if (editor_.ContainsPoint(dipX, dipY)) {
                ShowEditorContextMenu(dipX, dipY, pt.x, pt.y);
            }
            return 0;
        }

        case WM_MOUSEWHEEL:
            editor_.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;

        case WM_CHAR:
            editor_.OnChar(static_cast<wchar_t>(wParam));
            return 0;

        case WM_KEYDOWN: {
            UINT vk = static_cast<UINT>(wParam);
            if (vk == VK_ESCAPE) {
                HideToTray();
                return 0;
            }
            bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            editor_.OnKeyDown(vk, ctrl, shift);
            return 0;
        }

        case WM_SETFOCUS:
            editor_.SetFocused(true);
            return 0;

        case WM_KILLFOCUS:
            editor_.SetFocused(false);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

} // namespace korektabar
