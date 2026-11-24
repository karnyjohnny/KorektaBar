#include "SpellCheckerWin.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>

#if defined(__has_include) && __has_include(<spellcheck.h>)
#include <spellcheck.h>
#define KOREKTABAR_HAS_SPELLCHECK_HEADER 1
#else
#define KOREKTABAR_HAS_SPELLCHECK_HEADER 0
#endif

namespace korektabar {

#if !KOREKTABAR_HAS_SPELLCHECK_HEADER
// Minimalne definicje COM zgodne z ABI Windows 10 spellcheck.h (gdy starsze MinGW-w64 ich nie zawiera)
enum CORRECTIVE_ACTION {
    CORRECTIVE_ACTION_NONE = 0,
    CORRECTIVE_ACTION_GET_SUGGESTIONS = 1,
    CORRECTIVE_ACTION_REPLACE = 2,
    CORRECTIVE_ACTION_DELETE = 3
};

struct ISpellingError : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_StartIndex(ULONG* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Length(ULONG* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_CorrectiveAction(CORRECTIVE_ACTION* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Replacement(LPWSTR* value) = 0;
};

struct IEnumSpellingError : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Next(ISpellingError** value) = 0;
};

struct ISpellChecker : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_LanguageTag(LPWSTR* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE Check(LPCWSTR text, IEnumSpellingError** value) = 0;
    virtual HRESULT STDMETHODCALLTYPE Suggest(LPCWSTR word, IEnumString** value) = 0;
    virtual HRESULT STDMETHODCALLTYPE Add(LPCWSTR word) = 0;
    virtual HRESULT STDMETHODCALLTYPE Ignore(LPCWSTR word) = 0;
    virtual HRESULT STDMETHODCALLTYPE AutoCorrect(LPCWSTR from, LPCWSTR to) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetOptionValue(LPCWSTR optionId, BYTE* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_OptionIds(IEnumString** value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Id(LPWSTR* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_LocalizedName(LPWSTR* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_SpellCheckerChanged(IUnknown* handler, DWORD* eventCookie) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_SpellCheckerChanged(DWORD eventCookie) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetOptionDescription(LPCWSTR optionId, IUnknown** value) = 0;
    virtual HRESULT STDMETHODCALLTYPE ComprehensiveCheck(LPCWSTR text, IEnumSpellingError** value) = 0;
};

struct ISpellCheckerFactory : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_SupportedLanguages(IEnumString** value) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsSupported(LPCWSTR languageTag, BOOL* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateSpellChecker(LPCWSTR languageTag, ISpellChecker** value) = 0;
};
#endif

namespace {
const CLSID kClsidSpellCheckerFactory = {
    0x7ab36653, 0x1796, 0x484b, {0xbd, 0xfa, 0xe7, 0x4f, 0x1d, 0xb7, 0xc1, 0xdc}
};
const IID kIidSpellCheckerFactory = {
    0x8e018a9d, 0x2415, 0x4677, {0xbf, 0x08, 0x79, 0x4e, 0xa6, 0x1f, 0x94, 0xbb}
};
} // namespace

SpellCheckerWin::SpellCheckerWin() = default;

SpellCheckerWin::~SpellCheckerWin() {
    Shutdown();
}

bool SpellCheckerWin::Initialize() {
    if (checker_) return true;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) {
        comInitialized_ = true;
    } else if (hr == RPC_E_CHANGED_MODE) {
        comInitialized_ = false;
    }

    ISpellCheckerFactory* factory = nullptr;
    hr = CoCreateInstance(
        kClsidSpellCheckerFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        kIidSpellCheckerFactory,
        reinterpret_cast<void**>(&factory));

    if (FAILED(hr) || !factory) {
        return false;
    }

    BOOL supported = FALSE;
    hr = factory->IsSupported(L"pl-PL", &supported);
    if (SUCCEEDED(hr) && supported) {
        ISpellChecker* sc = nullptr;
        hr = factory->CreateSpellChecker(L"pl-PL", &sc);
        if (SUCCEEDED(hr) && sc) {
            checker_ = sc;
        }
    }

    factory->Release();
    return checker_ != nullptr;
}

void SpellCheckerWin::Shutdown() {
    if (checker_) {
        static_cast<ISpellChecker*>(checker_)->Release();
        checker_ = nullptr;
    }
    if (comInitialized_) {
        CoUninitialize();
        comInitialized_ = false;
    }
}

bool SpellCheckerWin::CheckWord(const std::wstring& word, std::vector<std::wstring>& outSuggestions) {
    outSuggestions.clear();
    if (!checker_ || word.empty()) return true;

    auto* sc = static_cast<ISpellChecker*>(checker_);
    IEnumSpellingError* enumErr = nullptr;
    HRESULT hr = sc->Check(word.c_str(), &enumErr);
    if (FAILED(hr) || !enumErr) return true;

    bool isCorrect = true;
    ISpellingError* err = nullptr;
    if (enumErr->Next(&err) == S_OK && err) {
        CORRECTIVE_ACTION action = CORRECTIVE_ACTION_NONE;
        err->get_CorrectiveAction(&action);

        if (action == CORRECTIVE_ACTION_REPLACE) {
            LPWSTR repl = nullptr;
            if (SUCCEEDED(err->get_Replacement(&repl)) && repl) {
                outSuggestions.emplace_back(repl);
                CoTaskMemFree(repl);
            }
            isCorrect = false;
        } else if (action == CORRECTIVE_ACTION_GET_SUGGESTIONS) {
            isCorrect = false;
            IEnumString* enumSugg = nullptr;
            if (SUCCEEDED(sc->Suggest(word.c_str(), &enumSugg)) && enumSugg) {
                LPOLESTR item = nullptr;
                ULONG fetched = 0;
                int count = 0;
                while (count < 5 && enumSugg->Next(1, &item, &fetched) == S_OK && fetched == 1) {
                    if (item) {
                        outSuggestions.emplace_back(item);
                        CoTaskMemFree(item);
                    }
                    ++count;
                }
                enumSugg->Release();
            }
        } else if (action == CORRECTIVE_ACTION_DELETE) {
            isCorrect = false;
        }
        err->Release();
    }

    enumErr->Release();
    return isCorrect;
}

} // namespace korektabar
