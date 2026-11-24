#pragma once

#include "PolishAnalyzer.hpp"

namespace korektabar {

// Wrapper na Windows 10 ISpellChecker COM API ("pl-PL").
// Bezpiecznie inicjalizowany wyłącznie w wątku, w którym jest używany.
class SpellCheckerWin final : public ISpellCheckBackend {
public:
    SpellCheckerWin();
    ~SpellCheckerWin() override;

    SpellCheckerWin(const SpellCheckerWin&) = delete;
    SpellCheckerWin& operator=(const SpellCheckerWin&) = delete;

    bool Initialize();
    void Shutdown();

    bool IsAvailable() const override { return checker_ != nullptr; }
    bool CheckWord(const std::wstring& word, std::vector<std::wstring>& outSuggestions) override;

private:
    void* checker_ = nullptr; // ISpellChecker*
    bool comInitialized_ = false;
};

} // namespace korektabar
