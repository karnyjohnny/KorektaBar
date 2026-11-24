#include "MainWindow.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int) {
    // Gwarancja jednej instancji procesu w zasobniku
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"KorektaBar_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(L"KorektaBarMainWindowClass", L"KorektaBar");
        if (existing) {
            ShowWindow(existing, SW_SHOW);
            SetForegroundWindow(existing);
        }
        if (mutex) CloseHandle(mutex);
        return 0;
    }

    // Per-Monitor V2 DPI Awareness (wsparcie programowe jako uzupełnienie manifestu)
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        using SetDpiCtxFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
        auto fn = reinterpret_cast<SetDpiCtxFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (fn) fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    korektabar::MainWindow app;
    if (!app.Create(hInstance)) {
        if (mutex) CloseHandle(mutex);
        return 1;
    }

    // Pokaż okno od razu jeśli uruchomiono z flagą --show, inaczej cichy start w zasobniku
    if (lpCmdLine && wcsstr(lpCmdLine, L"--show") != nullptr) {
        app.ShowAndFocus();
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (mutex) CloseHandle(mutex);
    return static_cast<int>(msg.wParam);
}
