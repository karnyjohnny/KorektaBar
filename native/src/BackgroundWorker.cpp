#include "BackgroundWorker.hpp"
#include "SpellCheckerWin.hpp"

namespace korektabar {

BackgroundWorker::~BackgroundWorker() {
    Stop();
}

void BackgroundWorker::Start(HWND notifyHwnd) {
    if (threadHandle_) return;
    notifyHwnd_ = notifyHwnd;
    wakeEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    stopEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    threadHandle_ = CreateThread(nullptr, 0, &BackgroundWorker::ThreadProc, this, 0, nullptr);
}

void BackgroundWorker::Stop() {
    if (stopEvent_) {
        SetEvent(stopEvent_);
    }
    if (threadHandle_) {
        WaitForSingleObject(threadHandle_, 2500);
        CloseHandle(threadHandle_);
        threadHandle_ = nullptr;
    }
    if (wakeEvent_) {
        CloseHandle(wakeEvent_);
        wakeEvent_ = nullptr;
    }
    if (stopEvent_) {
        CloseHandle(stopEvent_);
        stopEvent_ = nullptr;
    }
}

uint64_t BackgroundWorker::ScheduleAnalysis(
    std::wstring text,
    AnalysisOptions opts,
    std::unordered_set<std::wstring> ignoredWords)
{
    uint64_t rev = liveRevision_.fetch_add(1, std::memory_order_acq_rel) + 1;

    AcquireSRWLockExclusive(&inputLock_);
    pendingText_ = std::move(text);
    pendingOpts_ = opts;
    pendingIgnored_ = std::move(ignoredWords);
    hasPendingJob_ = true;
    ReleaseSRWLockExclusive(&inputLock_);

    if (wakeEvent_) SetEvent(wakeEvent_);
    return rev;
}

bool BackgroundWorker::ConsumeResults(uint64_t expectedRevision, std::vector<DiagnosticSpan>& outSpans) {
    if (expectedRevision != liveRevision_.load(std::memory_order_acquire)) {
        return false;
    }
    AcquireSRWLockShared(&outputLock_);
    bool match = (completedRevision_ == expectedRevision);
    if (match) {
        outSpans = completedSpans_;
    }
    ReleaseSRWLockShared(&outputLock_);
    return match;
}

DWORD WINAPI BackgroundWorker::ThreadProc(LPVOID param) {
    static_cast<BackgroundWorker*>(param)->RunLoop();
    return 0;
}

void BackgroundWorker::RunLoop() {
    // Inicjalizacja lekkiego COM ISpellChecker wyłącznie w wątku tła (nie blokuje UI)
    SpellCheckerWin spellChecker;
    spellChecker.Initialize();

    PolishAnalyzer analyzer;
    HANDLE handles[2] = {stopEvent_, wakeEvent_};

    while (true) {
        DWORD wait = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        if (wait == WAIT_OBJECT_0) {
            break; // stopEvent_
        }

        // Debounce 300 ms: czekaj aż użytkownik przestanie pisać (oszczędza CPU na Celeron N3350)
        while (true) {
            DWORD debounce = WaitForMultipleObjects(2, handles, FALSE, 300);
            if (debounce == WAIT_OBJECT_0) {
                return; // Zamykanie aplikacji
            }
            if (debounce == WAIT_TIMEOUT) {
                break; // 300 ms ciszy osiągnięte
            }
            // debounce == WAIT_OBJECT_0 + 1 -> przyszła kolejna zmiana tekstu, resetuj okno 300 ms
        }

        std::wstring textSnapshot;
        AnalysisOptions optsSnapshot{};
        std::unordered_set<std::wstring> ignoredSnapshot;
        uint64_t targetRev = 0;

        AcquireSRWLockExclusive(&inputLock_);
        if (!hasPendingJob_) {
            ReleaseSRWLockExclusive(&inputLock_);
            continue;
        }
        textSnapshot = pendingText_;
        optsSnapshot = pendingOpts_;
        ignoredSnapshot = pendingIgnored_;
        hasPendingJob_ = false;
        targetRev = liveRevision_.load(std::memory_order_acquire);
        ReleaseSRWLockExclusive(&inputLock_);

        std::vector<DiagnosticSpan> results = analyzer.Analyze(
            textSnapshot,
            optsSnapshot,
            ignoredSnapshot,
            spellChecker.IsAvailable() ? &spellChecker : nullptr,
            liveRevision_,
            targetRev);

        // Jeśli w trakcie analizy przyszła nowsza rewizja, porzuć przestarzałe wyniki
        if (liveRevision_.load(std::memory_order_acquire) != targetRev) {
            continue;
        }

        AcquireSRWLockExclusive(&outputLock_);
        completedRevision_ = targetRev;
        completedSpans_ = std::move(results);
        ReleaseSRWLockExclusive(&outputLock_);

        if (notifyHwnd_ && IsWindow(notifyHwnd_)) {
            PostMessageW(notifyHwnd_, WM_APP_DIAGNOSTICS_READY, static_cast<WPARAM>(targetRev), 0);
        }
    }
}

} // namespace korektabar
