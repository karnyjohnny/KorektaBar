#pragma once

#include "Types.hpp"

#include <atomic>
#include <string>
#include <unordered_set>
#include <vector>

namespace korektabar {

class ISpellCheckBackend {
public:
    virtual ~ISpellCheckBackend() = default;
    virtual bool IsAvailable() const = 0;
    virtual bool CheckWord(const std::wstring& word, std::vector<std::wstring>& outSuggestions) = 0;
};

struct AnalysisOptions {
    bool checkSpelling = true;
    bool checkPunctuation = true;
    bool checkStyle = true;
};

class PolishAnalyzer {
public:
    PolishAnalyzer() = default;

    std::vector<DiagnosticSpan> Analyze(
        const std::wstring& text,
        const AnalysisOptions& opts,
        const std::unordered_set<std::wstring>& ignoredWords,
        ISpellCheckBackend* spellBackend,
        const std::atomic<uint64_t>& liveRevision,
        uint64_t targetRevision) const;
};

} // namespace korektabar
