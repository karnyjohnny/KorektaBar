#include "TextFormatter.hpp"

#include <cwctype>

namespace korektabar {

namespace {

bool IsPolishUpper(wchar_t ch) {
    if (ch >= L'A' && ch <= L'Z') return true;
    switch (ch) {
        case L'Ą': case L'Ć': case L'Ę': case L'Ł': case L'Ń':
        case L'Ó': case L'Ś': case L'Ź': case L'Ż':
            return true;
        default:
            return false;
    }
}

bool IsPolishLower(wchar_t ch) {
    if (ch >= L'a' && ch <= L'z') return true;
    switch (ch) {
        case L'ą': case L'ć': case L'ę': case L'ł': case L'ń':
        case L'ó': case L'ś': case L'ź': case L'ż':
            return true;
        default:
            return false;
    }
}

bool IsPolishLetter(wchar_t ch) {
    return IsPolishUpper(ch) || IsPolishLower(ch);
}

wchar_t ToPolishUpper(wchar_t ch) {
    if (ch >= L'a' && ch <= L'z') return static_cast<wchar_t>(ch - L'a' + L'A');
    switch (ch) {
        case L'ą': return L'Ą';
        case L'ć': return L'Ć';
        case L'ę': return L'Ę';
        case L'ł': return L'Ł';
        case L'ń': return L'Ń';
        case L'ó': return L'Ó';
        case L'ś': return L'Ś';
        case L'ź': return L'Ź';
        case L'ż': return L'Ż';
        default: return ch;
    }
}

bool IsInlineSpace(wchar_t ch) {
    return ch == L' ' || ch == L'\t' || ch == L'\u00A0';
}

bool IsClosingPunctForNoLeadingSpace(wchar_t ch) {
    return ch == L',' || ch == L'.' || ch == L'!' || ch == L'?' ||
           ch == L';' || ch == L':' || ch == L')' || ch == L']';
}

bool NeedsSpaceAfterPunct(wchar_t ch) {
    return ch == L',' || ch == L'.' || ch == L'!' || ch == L'?' || ch == L';' || ch == L':';
}

} // namespace

std::wstring FormatPolishText(const std::wstring& input, const FormatterOptions& opts) {
    if (input.empty()) return L"";

    // Krok 1: Normalizacja końców linii do \n oraz obsługa spacji/akapitów
    std::wstring normalized;
    normalized.reserve(input.size() + 16);

    for (size_t i = 0; i < input.size(); ++i) {
        wchar_t ch = input[i];
        if (ch == L'\r') {
            if (i + 1 < input.size() && input[i + 1] == L'\n') ++i;
            normalized.push_back(L'\n');
        } else {
            normalized.push_back(ch);
        }
    }

    // Krok 2: Przebieg liniowy: redukcja spacji, czyszczenie końców linii i wielokrotnych pustych linii
    std::wstring pass1;
    pass1.reserve(normalized.size());

    int consecutiveNewlines = 0;
    bool prevWasSpace = false;
    bool atLineStart = true;

    for (size_t i = 0; i < normalized.size(); ++i) {
        wchar_t ch = normalized[i];
        if (ch == L'\n') {
            // Usuń spacje bezpośrednio przed końcem wiersza
            while (!pass1.empty() && IsInlineSpace(pass1.back())) {
                pass1.pop_back();
            }
            ++consecutiveNewlines;
            if (!opts.normalizeParagraphs || consecutiveNewlines <= 2) {
                pass1.push_back(L'\n');
            }
            prevWasSpace = false;
            atLineStart = true;
            continue;
        }

        consecutiveNewlines = 0;
        if (IsInlineSpace(ch)) {
            if (atLineStart && opts.collapseSpaces) {
                continue;
            }
            if (opts.collapseSpaces && prevWasSpace) {
                continue;
            }
            pass1.push_back(L' ');
            prevWasSpace = true;
            continue;
        }

        atLineStart = false;
        prevWasSpace = false;
        pass1.push_back(ch);
    }

    // Przytnij końcowe białe znaki
    while (!pass1.empty() && (IsInlineSpace(pass1.back()) || pass1.back() == L'\n')) {
        pass1.pop_back();
    }

    // Krok 3: Odstępy wokół znaków interpunkcyjnych
    std::wstring pass2;
    pass2.reserve(pass1.size() + 16);

    if (opts.fixPunctuationSpacing) {
        for (size_t i = 0; i < pass1.size(); ++i) {
            wchar_t ch = pass1[i];

            // Usuń spację przed znakami interpunkcyjnymi (, . ! ? ; : ) ])
            if (IsClosingPunctForNoLeadingSpace(ch)) {
                while (!pass2.empty() && pass2.back() == L' ') {
                    pass2.pop_back();
                }
                pass2.push_back(ch);

                // Dodaj brakującą spację po znaku interpunkcyjnym, jeśli następuje litera
                // (z wyjątkiem liczb dziesiętnych np. 3,14 oraz wielokropka ...)
                if (NeedsSpaceAfterPunct(ch) && i + 1 < pass1.size()) {
                    wchar_t next = pass1[i + 1];
                    bool isDecimal = (ch == L',' || ch == L'.') &&
                                     !pass2.empty() && pass2.size() >= 2 &&
                                     std::iswdigit(pass2[pass2.size() - 2]) && std::iswdigit(next);
                    bool isEllipsis = (ch == L'.' && next == L'.');
                    bool isRepeatedPunct = (next == L'.' || next == L'!' || next == L'?' ||
                                            next == L',' || next == L')' || next == L']' ||
                                            next == L'"' || next == L'\u201D');
                    if (!isDecimal && !isEllipsis && !isRepeatedPunct &&
                        next != L' ' && next != L'\n' && (IsPolishLetter(next) || std::iswdigit(next))) {
                        pass2.push_back(L' ');
                    }
                }
                continue;
            }

            // Usuń spację po nawiasie otwierającym ( lub [
            pass2.push_back(ch);
            if ((ch == L'(' || ch == L'[') && i + 1 < pass1.size() && pass1[i + 1] == L' ') {
                ++i;
            }
        }
    } else {
        pass2 = std::move(pass1);
    }

    // Krok 4: Wielkie litery na początku zdań i po znakach . ! ?
    if (opts.capitalizeSentences) {
        bool needCap = true;
        for (size_t i = 0; i < pass2.size(); ++i) {
            wchar_t ch = pass2[i];
            if (needCap) {
                if (IsPolishLetter(ch)) {
                    pass2[i] = ToPolishUpper(ch);
                    needCap = false;
                } else if (std::iswdigit(ch)) {
                    needCap = false;
                } else if (ch != L' ' && ch != L'\n' && ch != L'"' && ch != L'\u201E' && ch != L'(' && ch != L'-' && ch != L'\u2014') {
                    needCap = false;
                }
            } else {
                if (ch == L'.' || ch == L'!' || ch == L'?') {
                    // Sprawdź, czy to wielokropek lub skrót z małej litery typu "np." / "tzn." / liczba
                    bool isDecimal = (ch == L'.' && i > 0 && i + 1 < pass2.size() &&
                                      std::iswdigit(pass2[i - 1]) && std::iswdigit(pass2[i + 1]));
                    if (!isDecimal) {
                        needCap = true;
                    }
                } else if (ch == L'\n') {
                    // Nowy akapit po pustym wierszu
                    if (i > 0 && pass2[i - 1] == L'\n') {
                        needCap = true;
                    }
                }
            }
        }
    }

    return pass2;
}

} // namespace korektabar
