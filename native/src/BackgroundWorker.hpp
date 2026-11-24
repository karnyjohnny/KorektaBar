#pragma once

#include "PolishAnalyzer.hpp"
#include "Types.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <string>
#include <unordered_set>
#include <vector>

namespace korektabar {

class BackgroundWorker {
public:
    BackgroundWorker() = default;
    ~BackgroundWorker();

    BackgroundWorker(const BackgroundWorker&) = delete;
    BackgroundWorker& operator=(const BackgroundWorker&) = delete;

    void Start(HWND notifyHwnd);
    void Stop();

    // Zleca analizę po 300 ms debounce; natychmiast inkrementuje rewizję, by anulować poprzednią pracę.
    uint64_t ScheduleAnalysis(
        std::wstring text,
        AnalysisOptions opts,
        std::unordered_set<std::wstring> ignoredWords);

    uint64_t CurrentRevision() const {
        return liveRevision_.load(std::memory_order_relaxed);
    }

    // Pobiera wyniki dla konkretnej rewizji (wywoływane z wątku UI po WM_APP_DIAGNOSTICS_READY).
    bool ConsumeResults(uint64_t expectedRevision, std::vector<DiagnosticSpan>& outSpans);

private:
    static DWORD WINAPI ThreadProc(LPVOID param);
    void RunLoop();

    HWND notifyHwnd_ = nullptr;
    HANDLE threadHandle_ = nullptr;
    HANDLE wakeEvent_ = nullptr;
    HANDLE stopEvent_ = nullptr;

    SRWLOCK inputLock_ = SRWLOCK_INIT;
    std::wstring pendingText_;
    AnalysisOptions pendingOpts_{};
    std::unordered_set<std::wstring> pendingIgnored_;
    bool hasPendingJob_ = false;

    std::atomic<uint64_t> liveRevision_{0};

    SRWLOCK outputLock_ = SRWLOCK_INIT;
    uint64_t completedRevision_ = 0;
    std::vector<DiagnosticSpan> completedSpans_;
};

} // namespace korektabar
