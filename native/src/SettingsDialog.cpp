#include "SettingsDialog.hpp"
#include "Config.hpp"

#include <dwmapi.h>
#include <commctrl.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace korektabar {

namespace {

constexpr int IDC_HOTKEY_BOX = 2001;
constexpr int IDC_CHK_SPELLING = 2002;
constexpr int IDC_CHK_PUNCT = 2003;
constexpr int IDC_CHK_STYLE = 2004;
constexpr int IDC_CHK_SPACES = 2005;
constexpr int IDC_CHK_PUNCT_SPACE = 2006;
constexpr int IDC_CHK_PARAGRAPHS = 2007;
constexpr int IDC_CHK_CAPS = 2008;
constexpr int IDC_CHK_COPY_CLOSE = 2009;
constexpr int IDC_BTN_SAVE = 2010;
constexpr int IDC_BTN_CANCEL = 2011;

struct SettingsState {
    AppConfig working{};
    bool accepted = false;
    bool done = false;
    HFONT font = nullptr;
    HBRUSH bgBrush = nullptr;
    HBRUSH editBrush = nullptr;
    HWND hotkeyEdit = nullptr;
    WNDPROC origHotkeyProc = nullptr;
};

LRESULT CALLBACK HotkeyEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<SettingsState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) return DefWindowProcW(hwnd, msg, wParam, lParam);

    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        UINT vk = static_cast<UINT>(wParam);
        if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU || vk == VK_LWIN || vk == VK_RWIN) {
            return 0;
        }
        uint32_t mods = 0;
        if (GetKeyState(VK_CONTROL) & 0x8000) mods |= MOD_CONTROL;
        if (GetKeyState(VK_MENU) & 0x8000) mods |= MOD_ALT;
        if (GetKeyState(VK_SHIFT) & 0x8000) mods |= MOD_SHIFT;
        if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000)) mods |= MOD_WIN;

        if (mods == 0) mods = MOD_CONTROL; // Bezpieczny wymóg modyfikatora dla skrótu globalnego
        state->working.hotkeyModifiers = mods;
        state->working.hotkeyVk = vk;
        std::wstring text = ConfigManager::FormatHotkey(mods, vk);
        SetWindowTextW(hwnd, text.c_str());
        return 0;
    }
    if (msg == WM_CHAR || msg == WM_SYSCHAR) {
        return 0; // Blokuj wpisywanie znaków bezpośrednio
    }
    return CallWindowProcW(state->origHotkeyProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<SettingsState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            state = static_cast<SettingsState*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

            BOOL dark = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

            state->bgBrush = CreateSolidBrush(RGB(20, 22, 26));
            state->editBrush = CreateSolidBrush(RGB(27, 30, 36));
            state->font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            auto makeCtrl = [&](const wchar_t* cls, const wchar_t* txt, DWORD style,
                                int x, int y, int w, int h, int id) -> HWND {
                HWND c = CreateWindowExW(0, cls, txt, WS_CHILD | WS_VISIBLE | style,
                                         x, y, w, h, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                         GetModuleHandleW(nullptr), nullptr);
                SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
                return c;
            };

            makeCtrl(L"STATIC", L"Skrót globalny (kliknij pole i wciśnij kombinację):", 0, 20, 18, 360, 20, 0);
            state->hotkeyEdit = makeCtrl(L"EDIT",
                                         ConfigManager::FormatHotkey(state->working.hotkeyModifiers, state->working.hotkeyVk).c_str(),
                                         WS_BORDER | ES_READONLY | ES_CENTER,
                                         20, 42, 360, 28, IDC_HOTKEY_BOX);
            SetWindowLongPtrW(state->hotkeyEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
            state->origHotkeyProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrW(state->hotkeyEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&HotkeyEditProc)));

            makeCtrl(L"STATIC", L"Zakres analizy języka polskiego:", 0, 20, 84, 360, 20, 0);
            HWND c1 = makeCtrl(L"BUTTON", L"Ortografia i pułapki pisowni (pl-PL)", BS_AUTOCHECKBOX, 20, 108, 360, 22, IDC_CHK_SPELLING);
            HWND c2 = makeCtrl(L"BUTTON", L"Interpunkcja i gramatyka", BS_AUTOCHECKBOX, 20, 134, 360, 22, IDC_CHK_PUNCT);
            HWND c3 = makeCtrl(L"BUTTON", L"Styl, pleonazmy i powtórzenia", BS_AUTOCHECKBOX, 20, 160, 360, 22, IDC_CHK_STYLE);

            makeCtrl(L"STATIC", L"Reguły szybkiego formatera:", 0, 20, 196, 360, 20, 0);
            HWND c4 = makeCtrl(L"BUTTON", L"Redukuj nadmiarowe spacje", BS_AUTOCHECKBOX, 20, 220, 360, 22, IDC_CHK_SPACES);
            HWND c5 = makeCtrl(L"BUTTON", L"Porządkuj odstępy przy interpunkcji", BS_AUTOCHECKBOX, 20, 246, 360, 22, IDC_CHK_PUNCT_SPACE);
            HWND c6 = makeCtrl(L"BUTTON", L"Normalizuj puste linie i akapity", BS_AUTOCHECKBOX, 20, 272, 360, 22, IDC_CHK_PARAGRAPHS);
            HWND c7 = makeCtrl(L"BUTTON", L"Wielkie litery na początku zdań", BS_AUTOCHECKBOX, 20, 298, 360, 22, IDC_CHK_CAPS);
            HWND c8 = makeCtrl(L"BUTTON", L"Kopiuj i ukryj okno po „Auto-popraw”", BS_AUTOCHECKBOX, 20, 332, 360, 22, IDC_CHK_COPY_CLOSE);

            SendMessageW(c1, BM_SETCHECK, state->working.enableSpelling ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(c2, BM_SETCHECK, state->working.enablePunctuation ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(c3, BM_SETCHECK, state->working.enableStyle ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(c4, BM_SETCHECK, state->working.formatter.collapseSpaces ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(c5, BM_SETCHECK, state->working.formatter.fixPunctuationSpacing ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(c6, BM_SETCHECK, state->working.formatter.normalizeParagraphs ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(c7, BM_SETCHECK, state->working.formatter.capitalizeSentences ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(c8, BM_SETCHECK, state->working.copyAndCloseAfterAutoFix ? BST_CHECKED : BST_UNCHECKED, 0);

            makeCtrl(L"BUTTON", L"Zapisz", BS_DEFPUSHBUTTON, 196, 372, 90, 30, IDC_BTN_SAVE);
            makeCtrl(L"BUTTON", L"Anuluj", BS_PUSHBUTTON, 294, 372, 90, 30, IDC_BTN_CANCEL);
            return 0;
        }
        case WM_CTLCOLORDLG:
            return state ? reinterpret_cast<LRESULT>(state->bgBrush) : 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            if (state) {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                SetTextColor(hdc, RGB(236, 239, 244));
                SetBkColor(hdc, RGB(20, 22, 26));
                return reinterpret_cast<LRESULT>(state->bgBrush);
            }
            break;
        case WM_CTLCOLOREDIT:
            if (state) {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                SetTextColor(hdc, RGB(56, 189, 248));
                SetBkColor(hdc, RGB(27, 30, 36));
                return reinterpret_cast<LRESULT>(state->editBrush);
            }
            break;
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDC_BTN_SAVE && state) {
                auto isChecked = [&](int cid) {
                    return SendDlgItemMessageW(hwnd, cid, BM_GETCHECK, 0, 0) == BST_CHECKED;
                };
                state->working.enableSpelling = isChecked(IDC_CHK_SPELLING);
                state->working.enablePunctuation = isChecked(IDC_CHK_PUNCT);
                state->working.enableStyle = isChecked(IDC_CHK_STYLE);
                state->working.formatter.collapseSpaces = isChecked(IDC_CHK_SPACES);
                state->working.formatter.fixPunctuationSpacing = isChecked(IDC_CHK_PUNCT_SPACE);
                state->working.formatter.normalizeParagraphs = isChecked(IDC_CHK_PARAGRAPHS);
                state->working.formatter.capitalizeSentences = isChecked(IDC_CHK_CAPS);
                state->working.copyAndCloseAfterAutoFix = isChecked(IDC_CHK_COPY_CLOSE);
                state->accepted = true;
                state->done = true;
                DestroyWindow(hwnd);
                return 0;
            }
            if (id == IDC_BTN_CANCEL && state) {
                state->accepted = false;
                state->done = true;
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            if (state) {
                state->accepted = false;
                state->done = true;
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (state) {
                if (state->font) { DeleteObject(state->font); state->font = nullptr; }
                if (state->bgBrush) { DeleteObject(state->bgBrush); state->bgBrush = nullptr; }
                if (state->editBrush) { DeleteObject(state->editBrush); state->editBrush = nullptr; }
            }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

bool ShowSettingsDialog(HWND parentHwnd, AppConfig& inOutConfig) {
    static bool classRegistered = false;
    const wchar_t* kClass = L"KorektaBarSettingsClass";
    if (!classRegistered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = SettingsWndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = kClass;
        RegisterClassExW(&wc);
        classRegistered = true;
    }

    SettingsState state;
    state.working = inOutConfig;

    RECT parentRect{};
    GetWindowRect(parentHwnd, &parentRect);
    int dlgW = 420;
    int dlgH = 452;
    int x = parentRect.left + ((parentRect.right - parentRect.left) - dlgW) / 2;
    int y = std::max(20, static_cast<int>(parentRect.top) - dlgH / 2);

    EnableWindow(parentHwnd, FALSE);

    HWND dlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kClass,
        L"KorektaBar — Ustawienia",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, dlgW, dlgH,
        parentHwnd,
        nullptr,
        GetModuleHandleW(nullptr),
        &state);

    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);

    MSG msg{};
    while (!state.done && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE && GetActiveWindow() == dlg) {
            state.done = true;
            DestroyWindow(dlg);
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(parentHwnd, TRUE);
    SetForegroundWindow(parentHwnd);

    if (state.accepted) {
        inOutConfig = state.working;
        ConfigManager::Save(inOutConfig);
    }
    return state.accepted;
}

} // namespace korektabar
