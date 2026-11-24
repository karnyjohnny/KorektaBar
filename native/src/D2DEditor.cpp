#include "D2DEditor.hpp"

#include <algorithm>
#include <cmath>
#include <cwctype>

namespace korektabar {

namespace {
const IID kIidDWriteFactory = {
    0xb859ee5a, 0xd838, 0x4b5b, {0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48}
};

bool IsWordChar(wchar_t ch) {
    return std::iswalnum(ch) != 0 || ch == L'_' ||
           ch == L'ą' || ch == L'ć' || ch == L'ę' || ch == L'ł' || ch == L'ń' ||
           ch == L'ó' || ch == L'ś' || ch == L'ź' || ch == L'ż' ||
           ch == L'Ą' || ch == L'Ć' || ch == L'Ę' || ch == L'Ł' || ch == L'Ń' ||
           ch == L'Ó' || ch == L'Ś' || ch == L'Ź' || ch == L'Ż';
}

D2D1_COLOR_F ColorFromRgb(uint32_t rgb, float alpha = 1.0f) {
    float r = static_cast<float>((rgb >> 16) & 0xFF) / 255.0f;
    float g = static_cast<float>((rgb >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>(rgb & 0xFF) / 255.0f;
    return D2D1::ColorF(r, g, b, alpha);
}
} // namespace

D2DEditor::~D2DEditor() {
    Shutdown();
}

bool D2DEditor::Initialize(HWND parentHwnd, float dpi) {
    hwnd_ = parentHwnd;
    dpi_ = dpi > 0.0f ? dpi : 96.0f;

    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        kIidDWriteFactory,
        reinterpret_cast<IUnknown**>(&dwriteFactory_));
    if (FAILED(hr) || !dwriteFactory_) return false;

    hr = dwriteFactory_->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        15.0f,
        L"pl-PL",
        &textFormat_);
    if (FAILED(hr) || !textFormat_) return false;

    textFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    RebuildTextLayout();
    return true;
}

void D2DEditor::Shutdown() {
    if (textLayout_) { textLayout_->Release(); textLayout_ = nullptr; }
    if (textFormat_) { textFormat_->Release(); textFormat_ = nullptr; }
    if (dwriteFactory_) { dwriteFactory_->Release(); dwriteFactory_ = nullptr; }
}

void D2DEditor::SetBoundsDip(float x, float y, float width, float height) {
    left_ = x;
    top_ = y;
    width_ = std::max(120.0f, width);
    height_ = std::max(80.0f, height);
    RebuildTextLayout();
}

void D2DEditor::SetDpi(float dpi) {
    dpi_ = dpi > 0.0f ? dpi : 96.0f;
}

void D2DEditor::ResizeRenderTarget(UINT, UINT) {
    RebuildTextLayout();
}

void D2DEditor::ReleaseDeviceResources() {
    // Tworzymy pędzle lekko podczas Render lub trzymamy w MainWindow; layout jest niezależny od urządzenia
}

void D2DEditor::RebuildTextLayout() {
    if (!dwriteFactory_ || !textFormat_) return;
    if (textLayout_) {
        textLayout_->Release();
        textLayout_ = nullptr;
    }

    const float layoutWidth = std::max(40.0f, width_ - kPaddingX * 2.0f - 10.0f);
    HRESULT hr = dwriteFactory_->CreateTextLayout(
        text_.c_str(),
        static_cast<UINT32>(text_.size()),
        textFormat_,
        layoutWidth,
        10000.0f,
        &textLayout_);

    if (SUCCEEDED(hr) && textLayout_) {
        DWRITE_TEXT_METRICS metrics{};
        if (SUCCEEDED(textLayout_->GetMetrics(&metrics))) {
            contentHeight_ = metrics.height;
        }
        float maxScroll = std::max(0.0f, contentHeight_ - (height_ - kPaddingY * 2.0f));
        scrollY_ = std::clamp(scrollY_, 0.0f, maxScroll);
    }
}

void D2DEditor::Render(ID2D1RenderTarget* rt) {
    if (!rt) return;

    ID2D1SolidColorBrush* bgBrush = nullptr;
    ID2D1SolidColorBrush* borderBrush = nullptr;
    ID2D1SolidColorBrush* textBrush = nullptr;
    ID2D1SolidColorBrush* placeholderBrush = nullptr;
    ID2D1SolidColorBrush* selBrush = nullptr;
    ID2D1SolidColorBrush* caretBrush = nullptr;
    ID2D1SolidColorBrush* spellBrush = nullptr;
    ID2D1SolidColorBrush* punctBrush = nullptr;
    ID2D1SolidColorBrush* styleBrush = nullptr;
    ID2D1SolidColorBrush* scrollBrush = nullptr;

    rt->CreateSolidColorBrush(ColorFromRgb(0x1B1E24), &bgBrush);
    rt->CreateSolidColorBrush(ColorFromRgb(focused_ ? 0x3B82F6 : 0x2A2F3A), &borderBrush);
    rt->CreateSolidColorBrush(ColorFromRgb(0xECEFF4), &textBrush);
    rt->CreateSolidColorBrush(ColorFromRgb(0x64748B), &placeholderBrush);
    rt->CreateSolidColorBrush(ColorFromRgb(0x2563EB, 0.35f), &selBrush);
    rt->CreateSolidColorBrush(ColorFromRgb(0xECEFF4), &caretBrush);
    rt->CreateSolidColorBrush(ColorFromRgb(0xF87171), &spellBrush);
    rt->CreateSolidColorBrush(ColorFromRgb(0x38BDF8), &punctBrush);
    rt->CreateSolidColorBrush(ColorFromRgb(0xFBBF24), &styleBrush);
    rt->CreateSolidColorBrush(ColorFromRgb(0x475569, 0.65f), &scrollBrush);

    D2D1_ROUNDED_RECT box = D2D1::RoundedRect(
        D2D1::RectF(left_, top_, left_ + width_, top_ + height_), 8.0f, 8.0f);
    rt->FillRoundedRectangle(box, bgBrush);
    rt->DrawRoundedRectangle(box, borderBrush, 1.0f);

    // Przycinanie do obszaru pola tekstowego
    rt->PushAxisAlignedClip(
        D2D1::RectF(left_ + 4.0f, top_ + 4.0f, left_ + width_ - 4.0f, top_ + height_ - 4.0f),
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    const float originX = left_ + kPaddingX;
    const float originY = top_ + kPaddingY - scrollY_;

    // Rysowanie zaznaczenia
    if (textLayout_ && caret_ != anchor_) {
        uint32_t selStart = std::min(caret_, anchor_);
        uint32_t selLen = std::max(caret_, anchor_) - selStart;
        UINT32 count = 0;
        textLayout_->HitTestTextRange(selStart, selLen, originX, originY, nullptr, 0, &count);
        if (count > 0) {
            std::vector<DWRITE_HIT_TEST_METRICS> rects(count);
            if (SUCCEEDED(textLayout_->HitTestTextRange(selStart, selLen, originX, originY, rects.data(), count, &count))) {
                for (UINT32 i = 0; i < count; ++i) {
                    const auto& r = rects[i];
                    rt->FillRectangle(
                        D2D1::RectF(r.left, r.top, r.left + std::max(r.width, 4.0f), r.top + r.height),
                        selBrush);
                }
            }
        }
    }

    // Tekst główny lub placeholder
    if (text_.empty()) {
        const wchar_t* hint = L"Wpisz lub wklej polski tekst (Ctrl+V). Prawym przyciskiem myszy na podkreślonym słowie otworzysz sugestie…";
        if (textFormat_ && placeholderBrush) {
            rt->DrawTextW(
                hint,
                static_cast<UINT32>(wcslen(hint)),
                textFormat_,
                D2D1::RectF(originX, originY, left_ + width_ - kPaddingX, top_ + height_ - kPaddingY),
                placeholderBrush);
        }
    } else if (textLayout_ && textBrush) {
        rt->DrawTextLayout(D2D1::Point2F(originX, originY), textLayout_, textBrush);
    }

    // Rysowanie podkreśleń błędów (czerwone / błękitne / bursztynowe)
    if (textLayout_ && !diagnostics_.empty()) {
        for (const auto& sp : diagnostics_) {
            if (sp.start + sp.length > text_.size() || sp.length == 0) continue;
            ID2D1SolidColorBrush* b = spellBrush;
            if (sp.category == DiagnosticCategory::Punctuation) b = punctBrush;
            else if (sp.category == DiagnosticCategory::Style) b = styleBrush;

            UINT32 count = 0;
            textLayout_->HitTestTextRange(sp.start, sp.length, originX, originY, nullptr, 0, &count);
            if (count > 0) {
                std::vector<DWRITE_HIT_TEST_METRICS> rects(count);
                if (SUCCEEDED(textLayout_->HitTestTextRange(sp.start, sp.length, originX, originY, rects.data(), count, &count))) {
                    for (UINT32 i = 0; i < count; ++i) {
                        const auto& r = rects[i];
                        DrawSquiggle(rt, b, r.left, r.left + std::max(r.width, 6.0f), r.top + r.height - 1.5f);
                    }
                }
            }
        }
    }

    // Rysowanie kursora (caret)
    if (focused_ && caretVisible_ && textLayout_ && caretBrush) {
        FLOAT cx = 0.0f, cy = 0.0f;
        DWRITE_HIT_TEST_METRICS m{};
        if (SUCCEEDED(textLayout_->HitTestTextPosition(caret_, FALSE, &cx, &cy, &m))) {
            float drawX = originX + cx;
            float drawY = originY + m.top;
            float drawH = m.height > 0.0f ? m.height : 19.0f;
            rt->DrawLine(D2D1::Point2F(drawX, drawY + 1.0f), D2D1::Point2F(drawX, drawY + drawH - 1.0f), caretBrush, 1.5f);
        }
    }

    rt->PopAxisAlignedClip();

    // Pasek przewijania (tylko gdy tekst przekracza wysokość)
    float viewH = height_ - kPaddingY * 2.0f;
    if (contentHeight_ > viewH && viewH > 0.0f && scrollBrush) {
        float trackH = height_ - 16.0f;
        float thumbH = std::max(24.0f, trackH * (viewH / contentHeight_));
        float maxScroll = contentHeight_ - viewH;
        float ratio = maxScroll > 0.0f ? (scrollY_ / maxScroll) : 0.0f;
        float thumbY = top_ + 8.0f + ratio * (trackH - thumbH);
        rt->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(left_ + width_ - 7.0f, thumbY, left_ + width_ - 3.0f, thumbY + thumbH), 2.0f, 2.0f),
            scrollBrush);
    }

    if (bgBrush) bgBrush->Release();
    if (borderBrush) borderBrush->Release();
    if (textBrush) textBrush->Release();
    if (placeholderBrush) placeholderBrush->Release();
    if (selBrush) selBrush->Release();
    if (caretBrush) caretBrush->Release();
    if (spellBrush) spellBrush->Release();
    if (punctBrush) punctBrush->Release();
    if (styleBrush) styleBrush->Release();
    if (scrollBrush) scrollBrush->Release();
}

void D2DEditor::DrawSquiggle(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush, float x1, float x2, float y) {
    if (!brush || x2 <= x1) return;
    constexpr float step = 3.0f;
    constexpr float amp = 1.3f;
    float x = x1;
    bool up = true;
    D2D1_POINT_2F prev = D2D1::Point2F(x, y);
    while (x < x2) {
        float nextX = std::min(x2, x + step);
        float nextY = y + (up ? -amp : amp);
        D2D1_POINT_2F cur = D2D1::Point2F(nextX, nextY);
        rt->DrawLine(prev, cur, brush, 1.35f);
        prev = cur;
        x = nextX;
        up = !up;
    }
}

bool D2DEditor::ContainsPoint(float dipX, float dipY) const {
    return dipX >= left_ && dipX <= left_ + width_ && dipY >= top_ && dipY <= top_ + height_;
}

uint32_t D2DEditor::HitTestPointToIndex(float dipX, float dipY) const {
    if (!textLayout_ || text_.empty()) return 0;
    BOOL isTrailing = FALSE;
    BOOL isInside = FALSE;
    DWRITE_HIT_TEST_METRICS m{};
    float localX = dipX - (left_ + kPaddingX);
    float localY = dipY - (top_ + kPaddingY - scrollY_);
    textLayout_->HitTestPoint(localX, localY, &isTrailing, &isInside, &m);
    uint32_t idx = m.textPosition + (isTrailing ? 1 : 0);
    return std::min(idx, static_cast<uint32_t>(text_.size()));
}

void D2DEditor::OnMouseDown(float dipX, float dipY, bool shiftHeld) {
    uint32_t idx = HitTestPointToIndex(dipX, dipY);
    caret_ = idx;
    if (!shiftHeld) anchor_ = idx;
    dragging_ = true;
    caretVisible_ = true;
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::OnMouseDoubleClick(float dipX, float dipY) {
    uint32_t idx = HitTestPointToIndex(dipX, dipY);
    if (text_.empty()) return;
    uint32_t s = idx;
    uint32_t e = idx;
    while (s > 0 && IsWordChar(text_[s - 1])) --s;
    while (e < text_.size() && IsWordChar(text_[e])) ++e;
    anchor_ = s;
    caret_ = e;
    dragging_ = false;
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::OnMouseMove(float dipX, float dipY) {
    if (!dragging_) return;
    caret_ = HitTestPointToIndex(dipX, dipY);
    EnsureCaretVisible();
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::OnMouseUp() {
    dragging_ = false;
}

void D2DEditor::OnMouseWheel(int delta) {
    float viewH = height_ - kPaddingY * 2.0f;
    float maxScroll = std::max(0.0f, contentHeight_ - viewH);
    scrollY_ = std::clamp(scrollY_ - (static_cast<float>(delta) / 120.0f) * 32.0f, 0.0f, maxScroll);
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::EnsureCaretVisible() {
    if (!textLayout_) return;
    FLOAT cx = 0.0f, cy = 0.0f;
    DWRITE_HIT_TEST_METRICS m{};
    if (SUCCEEDED(textLayout_->HitTestTextPosition(caret_, FALSE, &cx, &cy, &m))) {
        float viewH = height_ - kPaddingY * 2.0f;
        if (m.top < scrollY_) {
            scrollY_ = m.top;
        } else if (m.top + m.height > scrollY_ + viewH) {
            scrollY_ = (m.top + m.height) - viewH;
        }
        float maxScroll = std::max(0.0f, contentHeight_ - viewH);
        scrollY_ = std::clamp(scrollY_, 0.0f, maxScroll);
    }
}

void D2DEditor::PushUndo() {
    undoStack_.push_back({text_, caret_, anchor_});
    if (undoStack_.size() > 100) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
}

void D2DEditor::ReplaceRangeInternal(uint32_t start, uint32_t len, const std::wstring& repl, bool recordUndo) {
    if (recordUndo) PushUndo();
    text_.replace(start, len, repl);
    caret_ = start + static_cast<uint32_t>(repl.size());
    anchor_ = caret_;
    diagnostics_.clear();
    RebuildTextLayout();
    EnsureCaretVisible();
    caretVisible_ = true;
    if (onTextChanged) onTextChanged();
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::DeleteSelectionInternal(bool recordUndo) {
    if (caret_ == anchor_) return;
    uint32_t s = std::min(caret_, anchor_);
    uint32_t e = std::max(caret_, anchor_);
    ReplaceRangeInternal(s, e - s, L"", recordUndo);
}

void D2DEditor::OnChar(wchar_t ch) {
    // Ignoruj znaki sterujące (Ctrl+A/C/V/X/Z/Y obsługiwane w OnKeyDown)
    if (ch < 32 && ch != L'\r' && ch != L'\t') return;
    if (ch == 127) return;

    std::wstring ins;
    if (ch == L'\r') ins = L"\n";
    else if (ch == L'\t') ins = L"    ";
    else ins = std::wstring(1, ch);

    uint32_t s = std::min(caret_, anchor_);
    uint32_t len = std::max(caret_, anchor_) - s;
    ReplaceRangeInternal(s, len, ins, true);
}

void D2DEditor::OnKeyDown(UINT vk, bool ctrl, bool shift) {
    if (ctrl) {
        if (vk == 'A') { SelectAll(); return; }
        if (vk == 'C') { CopyToClipboard(); return; }
        if (vk == 'X') { CutToClipboard(); return; }
        if (vk == 'V') { PasteFromClipboard(); return; }
        if (vk == 'Z') { Undo(); return; }
        if (vk == 'Y') { Redo(); return; }
    }

    if (vk == VK_LEFT) {
        if (ctrl) {
            uint32_t p = caret_;
            while (p > 0 && !IsWordChar(text_[p - 1])) --p;
            while (p > 0 && IsWordChar(text_[p - 1])) --p;
            caret_ = p;
        } else if (!shift && caret_ != anchor_) {
            caret_ = std::min(caret_, anchor_);
        } else if (caret_ > 0) {
            --caret_;
        }
        if (!shift) anchor_ = caret_;
    } else if (vk == VK_RIGHT) {
        if (ctrl) {
            uint32_t p = caret_;
            while (p < text_.size() && IsWordChar(text_[p])) ++p;
            while (p < text_.size() && !IsWordChar(text_[p])) ++p;
            caret_ = p;
        } else if (!shift && caret_ != anchor_) {
            caret_ = std::max(caret_, anchor_);
        } else if (caret_ < text_.size()) {
            ++caret_;
        }
        if (!shift) anchor_ = caret_;
    } else if (vk == VK_HOME) {
        uint32_t p = caret_;
        while (p > 0 && text_[p - 1] != L'\n') --p;
        caret_ = p;
        if (!shift) anchor_ = caret_;
    } else if (vk == VK_END) {
        uint32_t p = caret_;
        while (p < text_.size() && text_[p] != L'\n') ++p;
        caret_ = p;
        if (!shift) anchor_ = caret_;
    } else if (vk == VK_UP || vk == VK_DOWN) {
        if (textLayout_) {
            FLOAT cx = 0.0f, cy = 0.0f;
            DWRITE_HIT_TEST_METRICS m{};
            if (SUCCEEDED(textLayout_->HitTestTextPosition(caret_, FALSE, &cx, &cy, &m))) {
                float targetY = (top_ + kPaddingY - scrollY_) + m.top + (vk == VK_UP ? -m.height * 0.6f : m.height * 1.4f);
                float targetX = (left_ + kPaddingX) + cx;
                caret_ = HitTestPointToIndex(targetX, targetY);
                if (!shift) anchor_ = caret_;
            }
        }
    } else if (vk == VK_BACK) {
        if (caret_ != anchor_) {
            DeleteSelectionInternal(true);
            return;
        } else if (caret_ > 0) {
            uint32_t delStart = caret_ - 1;
            if (ctrl) {
                while (delStart > 0 && !IsWordChar(text_[delStart])) --delStart;
                while (delStart > 0 && IsWordChar(text_[delStart - 1])) --delStart;
            }
            ReplaceRangeInternal(delStart, caret_ - delStart, L"", true);
            return;
        }
    } else if (vk == VK_DELETE) {
        if (caret_ != anchor_) {
            DeleteSelectionInternal(true);
            return;
        } else if (caret_ < text_.size()) {
            uint32_t delEnd = caret_ + 1;
            if (ctrl) {
                while (delEnd < text_.size() && IsWordChar(text_[delEnd])) ++delEnd;
                while (delEnd < text_.size() && !IsWordChar(text_[delEnd])) ++delEnd;
            }
            ReplaceRangeInternal(caret_, delEnd - caret_, L"", true);
            return;
        }
    }

    EnsureCaretVisible();
    caretVisible_ = true;
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::SetText(const std::wstring& newText, bool recordUndo) {
    if (newText == text_) return;
    if (recordUndo) PushUndo();
    text_ = newText;
    caret_ = static_cast<uint32_t>(text_.size());
    anchor_ = caret_;
    diagnostics_.clear();
    RebuildTextLayout();
    EnsureCaretVisible();
    if (onTextChanged) onTextChanged();
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::SetDiagnostics(std::vector<DiagnosticSpan> spans) {
    diagnostics_ = std::move(spans);
    if (onRequestRepaint) onRequestRepaint();
}

const DiagnosticSpan* D2DEditor::HitTestDiagnostic(float dipX, float dipY) const {
    if (!ContainsPoint(dipX, dipY) || text_.empty()) return nullptr;
    uint32_t idx = HitTestPointToIndex(dipX, dipY);
    for (const auto& sp : diagnostics_) {
        if (idx >= sp.start && idx <= sp.start + sp.length) {
            return &sp;
        }
    }
    return nullptr;
}

bool D2DEditor::ApplyDiagnosticFix(const DiagnosticSpan& span, const std::wstring& replacement) {
    if (span.start + span.length > text_.size()) return false;
    ReplaceRangeInternal(span.start, span.length, replacement, true);
    return true;
}

int D2DEditor::AutoFixAll() {
    if (diagnostics_.empty()) return 0;

    // Zbieramy wyłącznie pewne poprawki i aplikujemy od końca tekstu (zachowanie spójności offsetów UTF-16)
    std::vector<DiagnosticSpan> fixable;
    for (const auto& sp : diagnostics_) {
        if (sp.autoFixable && !sp.suggestions.empty() && sp.start + sp.length <= text_.size()) {
            fixable.push_back(sp);
        }
    }
    if (fixable.empty()) return 0;

    std::sort(fixable.begin(), fixable.end(), [](const DiagnosticSpan& a, const DiagnosticSpan& b) {
        return a.start > b.start;
    });

    PushUndo();
    int applied = 0;
    uint32_t lastAppliedStart = static_cast<uint32_t>(text_.size() + 1);

    for (const auto& sp : fixable) {
        if (sp.start + sp.length <= lastAppliedStart) {
            text_.replace(sp.start, sp.length, sp.suggestions[0]);
            lastAppliedStart = sp.start;
            ++applied;
        }
    }

    if (applied > 0) {
        caret_ = static_cast<uint32_t>(text_.size());
        anchor_ = caret_;
        diagnostics_.clear();
        RebuildTextLayout();
        EnsureCaretVisible();
        if (onTextChanged) onTextChanged();
        if (onRequestRepaint) onRequestRepaint();
    }
    return applied;
}

void D2DEditor::Undo() {
    if (undoStack_.empty()) return;
    redoStack_.push_back({text_, caret_, anchor_});
    UndoEntry prev = std::move(undoStack_.back());
    undoStack_.pop_back();
    text_ = std::move(prev.text);
    caret_ = std::min(prev.caret, static_cast<uint32_t>(text_.size()));
    anchor_ = std::min(prev.anchor, static_cast<uint32_t>(text_.size()));
    diagnostics_.clear();
    RebuildTextLayout();
    EnsureCaretVisible();
    if (onTextChanged) onTextChanged();
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::Redo() {
    if (redoStack_.empty()) return;
    undoStack_.push_back({text_, caret_, anchor_});
    UndoEntry next = std::move(redoStack_.back());
    redoStack_.pop_back();
    text_ = std::move(next.text);
    caret_ = std::min(next.caret, static_cast<uint32_t>(text_.size()));
    anchor_ = std::min(next.anchor, static_cast<uint32_t>(text_.size()));
    diagnostics_.clear();
    RebuildTextLayout();
    EnsureCaretVisible();
    if (onTextChanged) onTextChanged();
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::SelectAll() {
    anchor_ = 0;
    caret_ = static_cast<uint32_t>(text_.size());
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::CopyToClipboard() {
    std::wstring slice = text_;
    if (caret_ != anchor_) {
        uint32_t s = std::min(caret_, anchor_);
        slice = text_.substr(s, std::max(caret_, anchor_) - s);
    }
    if (slice.empty()) return;

    if (!OpenClipboard(hwnd_)) return;
    EmptyClipboard();
    size_t bytes = (slice.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem) {
        void* ptr = GlobalLock(hMem);
        if (ptr) {
            memcpy(ptr, slice.c_str(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        } else {
            GlobalFree(hMem);
        }
    }
    CloseClipboard();
}

void D2DEditor::CutToClipboard() {
    if (caret_ == anchor_) return;
    CopyToClipboard();
    DeleteSelectionInternal(true);
}

void D2DEditor::PasteFromClipboard() {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return;
    if (!OpenClipboard(hwnd_)) return;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    std::wstring pasted;
    if (hData) {
        const wchar_t* ptr = static_cast<const wchar_t*>(GlobalLock(hData));
        if (ptr) {
            pasted = ptr;
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();

    if (pasted.empty()) return;
    // Normalizacja CRLF -> LF w buforze wewnętrznym dla spójnych offsetów UTF-16
    std::wstring clean;
    clean.reserve(pasted.size());
    for (size_t i = 0; i < pasted.size(); ++i) {
        if (pasted[i] == L'\r') {
            if (i + 1 < pasted.size() && pasted[i + 1] == L'\n') ++i;
            clean.push_back(L'\n');
        } else {
            clean.push_back(pasted[i]);
        }
    }

    uint32_t s = std::min(caret_, anchor_);
    uint32_t len = std::max(caret_, anchor_) - s;
    ReplaceRangeInternal(s, len, clean, true);
}

void D2DEditor::ToggleCaretBlink() {
    caretVisible_ = !caretVisible_;
    if (onRequestRepaint) onRequestRepaint();
}

void D2DEditor::SetFocused(bool focused) {
    focused_ = focused;
    caretVisible_ = focused;
    if (onRequestRepaint) onRequestRepaint();
}

} // namespace korektabar
