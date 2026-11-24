#pragma once

#include "Types.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include <functional>
#include <string>
#include <vector>

namespace korektabar {

class D2DEditor {
public:
    D2DEditor() = default;
    ~D2DEditor();

    D2DEditor(const D2DEditor&) = delete;
    D2DEditor& operator=(const D2DEditor&) = delete;

    bool Initialize(HWND parentHwnd, float dpi);
    void Shutdown();

    void SetBoundsDip(float x, float y, float width, float height);
    void SetDpi(float dpi);
    void ResizeRenderTarget(UINT pixelWidth, UINT pixelHeight);
    void ReleaseDeviceResources();

    void Render(ID2D1RenderTarget* rt);

    // Zdarzenia wejściowe z MainWindow (współrzędne w DIP względem okna rodzica)
    bool ContainsPoint(float dipX, float dipY) const;
    void OnMouseDown(float dipX, float dipY, bool shiftHeld);
    void OnMouseDoubleClick(float dipX, float dipY);
    void OnMouseMove(float dipX, float dipY);
    void OnMouseUp();
    void OnMouseWheel(int delta);

    void OnChar(wchar_t ch);
    void OnKeyDown(UINT vk, bool ctrl, bool shift);

    // Operacje na tekście i diagnostyce
    const std::wstring& GetText() const { return text_; }
    void SetText(const std::wstring& newText, bool recordUndo = true);
    void SetDiagnostics(std::vector<DiagnosticSpan> spans);
    const std::vector<DiagnosticSpan>& GetDiagnostics() const { return diagnostics_; }

    const DiagnosticSpan* HitTestDiagnostic(float dipX, float dipY) const;
    bool ApplyDiagnosticFix(const DiagnosticSpan& span, const std::wstring& replacement);
    int AutoFixAll();

    bool CanUndo() const { return !undoStack_.empty(); }
    bool CanRedo() const { return !redoStack_.empty(); }
    void Undo();
    void Redo();
    void CopyToClipboard();
    void CutToClipboard();
    void PasteFromClipboard();
    void SelectAll();

    void ToggleCaretBlink();
    void SetFocused(bool focused);

    std::function<void()> onTextChanged;
    std::function<void()> onRequestRepaint;

private:
    void RebuildTextLayout();
    void EnsureCaretVisible();
    void PushUndo();
    void ReplaceRangeInternal(uint32_t start, uint32_t len, const std::wstring& repl, bool recordUndo);
    void DeleteSelectionInternal(bool recordUndo);
    uint32_t HitTestPointToIndex(float dipX, float dipY) const;
    void DrawSquiggle(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush, float x1, float x2, float y);

    HWND hwnd_ = nullptr;
    float dpi_ = 96.0f;

    float left_ = 0.0f;
    float top_ = 0.0f;
    float width_ = 800.0f;
    float height_ = 160.0f;
    static constexpr float kPaddingX = 14.0f;
    static constexpr float kPaddingY = 12.0f;

    IDWriteFactory* dwriteFactory_ = nullptr;
    IDWriteTextFormat* textFormat_ = nullptr;
    IDWriteTextLayout* textLayout_ = nullptr;

    std::wstring text_;
    uint32_t caret_ = 0;
    uint32_t anchor_ = 0;
    float scrollY_ = 0.0f;
    float contentHeight_ = 0.0f;

    bool dragging_ = false;
    bool focused_ = true;
    bool caretVisible_ = true;

    std::vector<UndoEntry> undoStack_;
    std::vector<UndoEntry> redoStack_;
    std::vector<DiagnosticSpan> diagnostics_;
};

} // namespace korektabar
