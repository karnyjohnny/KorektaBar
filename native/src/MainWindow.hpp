#pragma once

#include "BackgroundWorker.hpp"
#include "D2DEditor.hpp"
#include "Types.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <shellapi.h>

#include <string>
#include <unordered_set>
#include <vector>

namespace korektabar {

class MainWindow {
public:
    MainWindow() = default;
    ~MainWindow();

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    bool Create(HINSTANCE hInstance);
    void ShowAndFocus();
    void HideToTray();
    void ToggleVisibility();

    HWND Handle() const { return hwnd_; }

private:
    struct ToolbarButton {
        int id = 0;
        std::wstring label;
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        bool primary = false;
    };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    bool EnsureGraphicsResources();
    void ReleaseGraphicsResources();
    void LayoutControls(float clientWidthDip, float clientHeightDip);
    void RepositionAboveTaskbar();
    void Render();

    void TriggerBackgroundAnalysis();
    void ApplyFastFormatter();
    void RunAutoFixAll();
    void CopyAndHide();
    void OpenSettings();
    void ShowEditorContextMenu(float dipX, float dipY, int screenX, int screenY);
    void ShowTrayMenu();
    void RegisterConfiguredHotkey();
    void AddTrayIcon();
    void RemoveTrayIcon();
    void TrimIdleMemory();

    HINSTANCE hInstance_ = nullptr;
    HWND hwnd_ = nullptr;
    float dpi_ = 96.0f;
    bool visible_ = false;

    AppConfig config_{};
    D2DEditor editor_;
    BackgroundWorker worker_;
    std::unordered_set<std::wstring> ignoredWords_;
    std::wstring statusText_ = L"Gotowy";

    std::vector<ToolbarButton> buttons_;
    int hoveredButtonId_ = 0;

    ID2D1Factory* d2dFactory_ = nullptr;
    ID2D1HwndRenderTarget* renderTarget_ = nullptr;
    IDWriteFactory* dwriteFactory_ = nullptr;
    IDWriteTextFormat* uiFont_ = nullptr;
    IDWriteTextFormat* uiBoldFont_ = nullptr;
    IDWriteTextFormat* statusFont_ = nullptr;

    NOTIFYICONDATAW nid_{};
    bool trayAdded_ = false;
};

} // namespace korektabar
