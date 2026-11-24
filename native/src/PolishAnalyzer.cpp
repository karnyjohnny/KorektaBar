#include "PolishAnalyzer.hpp"

#include <algorithm>
#include <cwctype>
#include <unordered_map>

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

wchar_t ToPolishLower(wchar_t ch) {
    if (ch >= L'A' && ch <= L'Z') return static_cast<wchar_t>(ch - L'A' + L'a');
    switch (ch) {
        case L'Ą': return L'ą';
        case L'Ć': return L'ć';
        case L'Ę': return L'ę';
        case L'Ł': return L'ł';
        case L'Ń': return L'ń';
        case L'Ó': return L'ó';
        case L'Ś': return L'ś';
        case L'Ź': return L'ź';
        case L'Ż': return L'ż';
        default: return ch;
    }
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

std::wstring LowercasePolish(const std::wstring& s) {
    std::wstring out;
    out.reserve(s.size());
    for (wchar_t c : s) out.push_back(ToPolishLower(c));
    return out;
}

std::wstring MatchCase(const std::wstring& original, const std::wstring& replacement) {
    if (original.empty() || replacement.empty()) return replacement;
    if (IsPolishUpper(original[0])) {
        std::wstring res = replacement;
        res[0] = ToPolishUpper(res[0]);
        return res;
    }
    return replacement;
}

struct WordToken {
    uint32_t start = 0;
    uint32_t length = 0;
    std::wstring text;
    std::wstring lower;
};

struct TrapEntry {
    const wchar_t* replacement;
    const wchar_t* title;
    const wchar_t* explanation;
    DiagnosticCategory category;
    bool autoFixable;
};

const std::unordered_map<std::wstring, TrapEntry>& GetWordTrapTable() {
    static const std::unordered_map<std::wstring, TrapEntry> table = {
        {L"wogóle", {L"w ogóle", L"Pisownia rozłączna", L"Wyrażenie przyimkowe „w ogóle” piszemy zawsze rozłącznie.", DiagnosticCategory::Spelling, true}},
        {L"wogole", {L"w ogóle", L"Pisownia rozłączna i znaki polskie", L"Poprawna forma to „w ogóle” (rozłącznie i z „ó”).", DiagnosticCategory::Spelling, true}},
        {L"wogule", {L"w ogóle", L"Błąd ortograficzny", L"Poprawna forma to „w ogóle”.", DiagnosticCategory::Spelling, true}},
        {L"napewno", {L"na pewno", L"Pisownia rozłączna", L"Wyrażenie „na pewno” piszemy rozłącznie.", DiagnosticCategory::Spelling, true}},
        {L"narazie", {L"na razie", L"Pisownia rozłączna", L"Wyrażenie „na razie” piszemy rozłącznie.", DiagnosticCategory::Spelling, true}},
        {L"spowrotem", {L"z powrotem", L"Pisownia rozłączna", L"Poprawna forma to „z powrotem”.", DiagnosticCategory::Spelling, true}},
        {L"zprzed", {L"sprzed", L"Błąd ortograficzny", L"Przyimek „sprzed” piszemy przez „s” łącznie.", DiagnosticCategory::Spelling, true}},
        {L"zkąd", {L"skąd", L"Błąd ortograficzny", L"Poprawna forma to „skąd”.", DiagnosticCategory::Spelling, true}},
        {L"wziąść", {L"wziąć", L"Błąd leksykalny", L"Czasownik brzmi „wziąć” (nie „wziąść”).", DiagnosticCategory::Spelling, true}},
        {L"wziąśc", {L"wziąć", L"Błąd leksykalny", L"Poprawna forma to „wziąć”.", DiagnosticCategory::Spelling, true}},
        {L"włanczać", {L"włączać", L"Błąd słowotwórczy", L"Poprawna forma to „włączać” (od „łączyć”).", DiagnosticCategory::Spelling, true}},
        {L"włanczam", {L"włączam", L"Błąd słowotwórczy", L"Poprawna forma to „włączam”.", DiagnosticCategory::Spelling, true}},
        {L"włancza", {L"włącza", L"Błąd słowotwórczy", L"Poprawna forma to „włącza”.", DiagnosticCategory::Spelling, true}},
        {L"wyłanczać", {L"wyłączać", L"Błąd słowotwórczy", L"Poprawna forma to „wyłączać”.", DiagnosticCategory::Spelling, true}},
        {L"poszłem", {L"poszedłem", L"Błąd fleksyjny", L"Poprawna forma czasu przeszłego rodzaju męskiego to „poszedłem”.", DiagnosticCategory::Punctuation, true}},
        {L"szłem", {L"szedłem", L"Błąd fleksyjny", L"Poprawna forma to „szedłem”.", DiagnosticCategory::Punctuation, true}},
        {L"przyszłem", {L"przyszedłem", L"Błąd fleksyjny", L"Poprawna forma to „przyszedłem”.", DiagnosticCategory::Punctuation, true}},
        {L"wyszłem", {L"wyszedłem", L"Błąd fleksyjny", L"Poprawna forma to „wyszedłem”.", DiagnosticCategory::Punctuation, true}},
        {L"przynajmiej", {L"przynajmniej", L"Literówka", L"Brakuje litery „n”: „przynajmniej”.", DiagnosticCategory::Spelling, true}},
        {L"bynajmiej", {L"bynajmniej", L"Literówka", L"Brakuje litery „n”: „bynajmniej”.", DiagnosticCategory::Spelling, true}},
        {L"pujść", {L"pójść", L"Błąd ortograficzny", L"Poprawna forma to „pójść” przez „ó”.", DiagnosticCategory::Spelling, true}},
        {L"pójśc", {L"pójść", L"Brak znaku diakrytycznego", L"Poprawna forma to „pójść”.", DiagnosticCategory::Spelling, true}},
        {L"kłucić", {L"kłócić", L"Błąd ortograficzny", L"Poprawna forma to „kłócić” przez „ó”.", DiagnosticCategory::Spelling, true}},
        {L"kłucic", {L"kłócić", L"Błąd ortograficzny", L"Poprawna forma to „kłócić”.", DiagnosticCategory::Spelling, true}},
        {L"żecz", {L"rzecz", L"Błąd ortograficzny", L"Słowo „rzecz” piszemy przez „rz”.", DiagnosticCategory::Spelling, true}},
        {L"żeczy", {L"rzeczy", L"Błąd ortograficzny", L"Słowo „rzeczy” piszemy przez „rz”.", DiagnosticCategory::Spelling, true}},
        {L"żeczywiscie", {L"rzeczywiście", L"Błąd ortograficzny", L"Poprawna forma to „rzeczywiście”.", DiagnosticCategory::Spelling, true}},
        {L"rzeczywiscie", {L"rzeczywiście", L"Brak polskiego znaku", L"Poprawna forma to „rzeczywiście”.", DiagnosticCategory::Spelling, true}},
        {L"oczywiscie", {L"oczywiście", L"Brak polskiego znaku", L"Poprawna forma to „oczywiście”.", DiagnosticCategory::Spelling, true}},
        {L"szczegulnie", {L"szczególnie", L"Błąd ortograficzny", L"Poprawna forma to „szczególnie” przez „ó”.", DiagnosticCategory::Spelling, true}},
        {L"szczegolnie", {L"szczególnie", L"Brak polskiego znaku", L"Poprawna forma to „szczególnie”.", DiagnosticCategory::Spelling, true}},
        {L"ogulnie", {L"ogólnie", L"Błąd ortograficzny", L"Poprawna forma to „ogólnie” przez „ó”.", DiagnosticCategory::Spelling, true}},
        {L"ogolnie", {L"ogólnie", L"Brak polskiego znaku", L"Poprawna forma to „ogólnie”.", DiagnosticCategory::Spelling, true}},
        {L"póżniej", {L"później", L"Brak znaku diakrytycznego", L"Poprawna forma to „później”.", DiagnosticCategory::Spelling, true}},
        {L"puzniej", {L"później", L"Błąd ortograficzny", L"Poprawna forma to „później” przez „óż”.", DiagnosticCategory::Spelling, true}},
        {L"pozniej", {L"później", L"Brak polskich znaków", L"Poprawna forma to „później”.", DiagnosticCategory::Spelling, true}},
        {L"wczesniej", {L"wcześniej", L"Brak polskiego znaku", L"Poprawna forma to „wcześniej”.", DiagnosticCategory::Spelling, true}},
        {L"wlasnie", {L"właśnie", L"Brak polskich znaków", L"Poprawna forma to „właśnie”.", DiagnosticCategory::Spelling, true}},
        {L"ktory", {L"który", L"Brak polskiego znaku", L"Poprawna forma to „który”.", DiagnosticCategory::Spelling, true}},
        {L"ktora", {L"która", L"Brak polskiego znaku", L"Poprawna forma to „która”.", DiagnosticCategory::Spelling, true}},
        {L"ktore", {L"które", L"Brak polskiego znaku", L"Poprawna forma to „które”.", DiagnosticCategory::Spelling, true}},
        {L"ktorych", {L"których", L"Brak polskiego znaku", L"Poprawna forma to „których”.", DiagnosticCategory::Spelling, true}},
        {L"ktorym", {L"którym", L"Brak polskiego znaku", L"Poprawna forma to „którym”.", DiagnosticCategory::Spelling, true}},
        {L"ktorzy", {L"którzy", L"Brak polskiego znaku", L"Poprawna forma to „którzy”.", DiagnosticCategory::Spelling, true}},
        {L"poniewaz", {L"ponieważ", L"Brak polskiego znaku", L"Poprawna forma to „ponieważ”.", DiagnosticCategory::Spelling, true}},
        {L"rowniez", {L"również", L"Brak polskich znaków", L"Poprawna forma to „również”.", DiagnosticCategory::Spelling, true}},
        {L"takze", {L"także", L"Brak polskiego znaku", L"Poprawna forma to „także”.", DiagnosticCategory::Spelling, true}},
        {L"mozna", {L"można", L"Brak polskiego znaku", L"Poprawna forma to „można”.", DiagnosticCategory::Spelling, true}},
        {L"morzna", {L"można", L"Błąd ortograficzny", L"Poprawna forma to „można” przez „ż”.", DiagnosticCategory::Spelling, true}},
        {L"wiecej", {L"więcej", L"Brak polskiego znaku", L"Poprawna forma to „więcej”.", DiagnosticCategory::Spelling, true}},
        {L"bedzie", {L"będzie", L"Brak polskiego znaku", L"Poprawna forma to „będzie”.", DiagnosticCategory::Spelling, true}},
        {L"bede", {L"będę", L"Brak polskich znaków", L"Poprawna forma to „będę”.", DiagnosticCategory::Spelling, true}},
        {L"jestes", {L"jesteś", L"Brak polskiego znaku", L"Poprawna forma to „jesteś”.", DiagnosticCategory::Spelling, true}},
        {L"mozemy", {L"możemy", L"Brak polskiego znaku", L"Poprawna forma to „możemy”.", DiagnosticCategory::Spelling, true}},
        {L"musze", {L"muszę", L"Brak polskiego znaku", L"Poprawna forma to „muszę”.", DiagnosticCategory::Spelling, true}},
        {L"dziekuje", {L"dziękuję", L"Brak polskich znaków", L"Poprawna forma to „dziękuję”.", DiagnosticCategory::Spelling, true}},
        {L"prosze", {L"proszę", L"Brak polskiego znaku", L"Poprawna forma to „proszę”.", DiagnosticCategory::Spelling, true}},
        {L"ksiazka", {L"książka", L"Brak polskich znaków", L"Poprawna forma to „książka”.", DiagnosticCategory::Spelling, true}},
        {L"pieniadze", {L"pieniądze", L"Brak polskiego znaku", L"Poprawna forma to „pieniądze”.", DiagnosticCategory::Spelling, true}},
        {L"wiadomosc", {L"wiadomość", L"Brak polskich znaków", L"Poprawna forma to „wiadomość”.", DiagnosticCategory::Spelling, true}},
        {L"wiadomosci", {L"wiadomości", L"Brak polskiego znaku", L"Poprawna forma to „wiadomości”.", DiagnosticCategory::Spelling, true}},
        {L"bład", {L"błąd", L"Brak polskiego znaku", L"Poprawna forma to „błąd”.", DiagnosticCategory::Spelling, true}},
        {L"błond", {L"błąd", L"Błąd ortograficzny", L"Poprawna forma to „błąd” przez „ą”.", DiagnosticCategory::Spelling, true}},
        {L"duzo", {L"dużo", L"Brak polskiego znaku", L"Poprawna forma to „dużo”.", DiagnosticCategory::Spelling, true}},
        {L"zolty", {L"żółty", L"Brak polskich znaków", L"Poprawna forma to „żółty”.", DiagnosticCategory::Spelling, true}},
        {L"zrodlo", {L"źródło", L"Brak polskich znaków", L"Poprawna forma to „źródło”.", DiagnosticCategory::Spelling, true}},
        {L"juz", {L"już", L"Brak polskiego znaku", L"Poprawna forma to „już”.", DiagnosticCategory::Spelling, true}},
        {L"cos", {L"coś", L"Brak polskiego znaku", L"Poprawna forma to „coś”.", DiagnosticCategory::Spelling, true}},
        {L"ktos", {L"ktoś", L"Brak polskiego znaku", L"Poprawna forma to „ktoś”.", DiagnosticCategory::Spelling, true}},
        {L"gdzies", {L"gdzieś", L"Brak polskiego znaku", L"Poprawna forma to „gdzieś”.", DiagnosticCategory::Spelling, true}},
        {L"kiedys", {L"kiedyś", L"Brak polskiego znaku", L"Poprawna forma to „kiedyś”.", DiagnosticCategory::Spelling, true}},
        {L"przegladarka", {L"przeglądarka", L"Brak polskiego znaku", L"Poprawna forma to „przeglądarka”.", DiagnosticCategory::Spelling, true}},
        {L"zdjecie", {L"zdjęcie", L"Brak polskiego znaku", L"Poprawna forma to „zdjęcie”.", DiagnosticCategory::Spelling, true}},
        {L"pisze", {L"piszę / jest napisane", L"Uwaga stylistyczna", L"W znaczeniu „w tekście widnieje” poprawniej jest użyć formy „jest napisane” lub „napisano”.", DiagnosticCategory::Style, false}},
    };
    return table;
}

bool IsSubordinateConjunction(const std::wstring& lower) {
    static const std::unordered_set<std::wstring> words = {
        L"że", L"żeby", L"aby", L"iż", L"ponieważ", L"gdyż", L"albowiem",
        L"chociaż", L"choć", L"dopóki", L"zanim", L"który", L"która",
        L"które", L"których", L"którym", L"którą", L"którzy", L"gdzie", L"kiedy"
    };
    return words.count(lower) > 0;
}

bool IsCompoundCommaPrefix(const std::wstring& prevLower, const std::wstring& curLower) {
    if (curLower == L"że" || curLower == L"żeby" || curLower == L"aby") {
        if (prevLower == L"mimo" || prevLower == L"pomimo" || prevLower == L"chyba" ||
            prevLower == L"dlatego" || prevLower == L"tym" || prevLower == L"jako") {
            return true;
        }
    }
    if (curLower == L"kiedy" && (prevLower == L"od" || prevLower == L"wtedy")) {
        return true;
    }
    if (curLower == L"gdzie" && prevLower == L"tam") {
        return true;
    }
    return false;
}

bool IsCoordinatingConjunctionNoComma(const std::wstring& lower) {
    return lower == L"i" || lower == L"oraz" || lower == L"lub" || lower == L"albo" || lower == L"czy";
}

bool IsPunctNoSpaceBefore(wchar_t ch) {
    return ch == L',' || ch == L'.' || ch == L'!' || ch == L'?' || ch == L';' || ch == L':';
}

bool Overlaps(uint32_t s1, uint32_t l1, uint32_t s2, uint32_t l2) {
    return s1 < s2 + l2 && s2 < s1 + l1;
}

} // namespace

std::vector<DiagnosticSpan> PolishAnalyzer::Analyze(
    const std::wstring& text,
    const AnalysisOptions& opts,
    const std::unordered_set<std::wstring>& ignoredWords,
    ISpellCheckBackend* spellBackend,
    const std::atomic<uint64_t>& liveRevision,
    uint64_t targetRevision) const
{
    std::vector<DiagnosticSpan> spans;
    if (text.empty()) return spans;

    auto cancelled = [&]() -> bool {
        return liveRevision.load(std::memory_order_relaxed) != targetRevision;
    };

    // 1. Tokenizacja wyrazów UTF-16
    std::vector<WordToken> tokens;
    tokens.reserve(text.size() / 5 + 8);

    uint32_t i = 0;
    const uint32_t n = static_cast<uint32_t>(text.size());
    while (i < n) {
        if (cancelled()) return {};
        if (IsPolishLetter(text[i])) {
            uint32_t start = i;
            while (i < n && (IsPolishLetter(text[i]) || text[i] == L'-')) {
                // Nie traktuj myślnika na końcu wyrazu jako części tokenu
                if (text[i] == L'-' && (i + 1 >= n || !IsPolishLetter(text[i + 1]))) break;
                ++i;
            }
            WordToken tok;
            tok.start = start;
            tok.length = i - start;
            tok.text = text.substr(start, tok.length);
            tok.lower = LowercasePolish(tok.text);
            tokens.push_back(std::move(tok));
        } else {
            ++i;
        }
    }

    // 2. Błędy wielowyrazowe / frazeologiczne i pleonazmy
    for (size_t t = 0; t + 1 < tokens.size(); ++t) {
        if (cancelled()) return {};
        const auto& a = tokens[t];
        const auto& b = tokens[t + 1];
        // Sprawdź czy między słowami jest tylko pojedyncza spacja
        if (b.start - (a.start + a.length) <= 2) {
            std::wstring gap = text.substr(a.start + a.length, b.start - (a.start + a.length));
            if (gap == L" ") {
                std::wstring pair = a.lower + L" " + b.lower;
                uint32_t pairLen = (b.start + b.length) - a.start;

                if (opts.checkSpelling && pair == L"po za") {
                    spans.push_back({a.start, pairLen, DiagnosticCategory::Spelling,
                                     L"Pisownia łączna", L"Przyimek „poza” piszemy łącznie.",
                                     {MatchCase(a.text, L"poza")}, true});
                } else if (opts.checkSpelling && pair == L"dla tego") {
                    spans.push_back({a.start, pairLen, DiagnosticCategory::Style,
                                     L"Możliwy błąd łączności", L"Jeśli oznacza wynik/przyczynę, piszemy łącznie „dlatego”.",
                                     {MatchCase(a.text, L"dlatego")}, false});
                } else if (opts.checkStyle && pair == L"okres czasu") {
                    spans.push_back({a.start, pairLen, DiagnosticCategory::Style,
                                     L"Pleonazm („masło maślane”)", L"Słowo „okres” już oznacza wycinek czasu.",
                                     {MatchCase(a.text, L"okres")}, true});
                } else if (opts.checkStyle && pair == L"akwen wodny") {
                    spans.push_back({a.start, pairLen, DiagnosticCategory::Style,
                                     L"Pleonazm", L"„Akwen” z definicji jest obszarem wodnym.",
                                     {MatchCase(a.text, L"akwen")}, true});
                } else if (opts.checkStyle && pair == L"fakt autentyczny") {
                    spans.push_back({a.start, pairLen, DiagnosticCategory::Style,
                                     L"Pleonazm", L"Każdy fakt jest autentyczny — wystarczy „fakt”.",
                                     {MatchCase(a.text, L"fakt")}, true});
                } else if (opts.checkStyle && pair == L"dzień dzisiejszy") {
                    spans.push_back({a.start, pairLen, DiagnosticCategory::Style,
                                     L"Złożoność stylistyczna", L"Zamiast „dzień dzisiejszy” lepiej użyć „dzisiaj” lub „obecnie”.",
                                     {MatchCase(a.text, L"dzisiaj")}, false});
                }

                // 3-wyrazowe pleonazmy
                if (t + 2 < tokens.size() && opts.checkStyle) {
                    const auto& c = tokens[t + 2];
                    if (c.start - (b.start + b.length) == 1 && text[b.start + b.length] == L' ') {
                        std::wstring tri = pair + L" " + c.lower;
                        uint32_t triLen = (c.start + c.length) - a.start;
                        if (tri == L"spadać w dół") {
                            spans.push_back({a.start, triLen, DiagnosticCategory::Style,
                                             L"Pleonazm", L"Spadać można wyłącznie w dół.",
                                             {MatchCase(a.text, L"spadać")}, true});
                        } else if (tri == L"cofać się wstecz" || tri == L"cofnąć się wstecz") {
                            spans.push_back({a.start, triLen, DiagnosticCategory::Style,
                                             L"Pleonazm", L"Cofanie zawsze odbywa się wstecz.",
                                             {MatchCase(a.text, a.lower + L" się")}, true});
                        }
                    }
                }
            }
        }
    }

    // 3. Pułapki wyrazowe + ISpellChecker + powtórzenia sąsiednie
    const auto& traps = GetWordTrapTable();
    for (size_t t = 0; t < tokens.size(); ++t) {
        if (cancelled()) return {};
        const auto& tok = tokens[t];
        if (ignoredWords.count(tok.lower) > 0) continue;

        // Sąsiednie powtórzenie tego samego wyrazu (np. "jest jest")
        if (opts.checkStyle && t > 0) {
            const auto& prev = tokens[t - 1];
            if (prev.lower == tok.lower) {
                std::wstring between = text.substr(prev.start + prev.length, tok.start - (prev.start + prev.length));
                if (between == L" ") {
                    spans.push_back({prev.start, (tok.start + tok.length) - prev.start,
                                     DiagnosticCategory::Style,
                                     L"Powtórzenie wyrazu",
                                     L"Zdublowane słowo „" + tok.text + L"”.",
                                     {prev.text}, true});
                    continue;
                }
            }
        }

        // Tabela pułapek polskich
        auto it = traps.find(tok.lower);
        if (it != traps.end()) {
            const TrapEntry& entry = it->second;
            bool enabled = (entry.category == DiagnosticCategory::Spelling && opts.checkSpelling) ||
                           (entry.category == DiagnosticCategory::Punctuation && opts.checkPunctuation) ||
                           (entry.category == DiagnosticCategory::Style && opts.checkStyle);
            if (enabled) {
                spans.push_back({tok.start, tok.length, entry.category,
                                 entry.title, entry.explanation,
                                 {MatchCase(tok.text, entry.replacement)},
                                 entry.autoFixable});
            }
            continue;
        }

        // Bliskie powtórzenie dłuższego wyrazu w oknie 5 słów
        if (opts.checkStyle && tok.length >= 5) {
            for (size_t back = 1; back <= 5 && t >= back; ++back) {
                if (tokens[t - back].lower == tok.lower) {
                    spans.push_back({tok.start, tok.length, DiagnosticCategory::Style,
                                     L"Bliskie powtórzenie",
                                     L"Wyraz „" + tok.text + L"” pojawił się przed chwilą w tym samym fragmencie.",
                                     {}, false});
                    break;
                }
            }
        }

        // Zewnętrzny słownik ortograficzny Windows ISpellChecker (pl-PL)
        if (opts.checkSpelling && spellBackend && spellBackend->IsAvailable() && tok.length >= 2) {
            // Nie sprawdzaj wyrazów pisanych w całości WIELKIMI LITERAMI (akronimy typu CPU, RAM)
            bool allCaps = std::all_of(tok.text.begin(), tok.text.end(), IsPolishUpper);
            if (!allCaps) {
                std::vector<std::wstring> sugg;
                if (!spellBackend->CheckWord(tok.text, sugg)) {
                    spans.push_back({tok.start, tok.length, DiagnosticCategory::Spelling,
                                     L"Nieznane słowo / literówka",
                                     L"Słowo „" + tok.text + L"” nie występuje w słowniku pl-PL.",
                                     std::move(sugg), false});
                }
            }
        }
    }

    // 4. Reguły interpunkcji polskiej (przecinki przed spójnikami, spacje wokół znaków)
    if (opts.checkPunctuation) {
        for (size_t t = 1; t < tokens.size(); ++t) {
            if (cancelled()) return {};
            const auto& prev = tokens[t - 1];
            const auto& cur = tokens[t];
            uint32_t gapStart = prev.start + prev.length;
            uint32_t gapLen = cur.start - gapStart;
            std::wstring gap = text.substr(gapStart, gapLen);

            if (IsSubordinateConjunction(cur.lower) && !IsCompoundCommaPrefix(prev.lower, cur.lower)) {
                // Brak przecinka jeśli między słowami są wyłącznie spacje
                bool onlySpaces = !gap.empty() && std::all_of(gap.begin(), gap.end(), [](wchar_t c) { return c == L' '; });
                if (onlySpaces) {
                    spans.push_back({cur.start, cur.length, DiagnosticCategory::Punctuation,
                                     L"Brakujący przecinek",
                                     L"Przed spójnikiem podrzędnym „" + cur.lower + L"” stawiamy zazwyczaj przecinek.",
                                     {L", " + cur.text}, false});
                }
            }

            // Zbędny przecinek przed pojedynczym "i / lub / albo / czy"
            if (IsCoordinatingConjunctionNoComma(cur.lower)) {
                if (gap.find(L',') != std::wstring::npos && gap.find(L'\n') == std::wstring::npos) {
                    uint32_t commaPos = static_cast<uint32_t>(gapStart + gap.find(L','));
                    spans.push_back({commaPos, 1, DiagnosticCategory::Punctuation,
                                     L"Zbędny przecinek",
                                     L"Przed pojedynczym spójnikiem „" + cur.lower + L"” zwykle nie stawiamy przecinka.",
                                     {L""}, false});
                }
            }
        }

        // Spacje przed interpunkcją i brak spacji po interpunkcji
        for (uint32_t k = 0; k < n; ++k) {
            if (cancelled()) return {};
            wchar_t ch = text[k];
            if (IsPunctNoSpaceBefore(ch) && k > 0 && text[k - 1] == L' ') {
                uint32_t spStart = k - 1;
                while (spStart > 0 && text[spStart - 1] == L' ') --spStart;
                uint32_t len = (k + 1) - spStart;
                spans.push_back({spStart, len, DiagnosticCategory::Punctuation,
                                 L"Spacja przed interpunkcją",
                                 L"Przed znakiem „" + std::wstring(1, ch) + L"” nie stawiamy spacji.",
                                 {std::wstring(1, ch)}, true});
            }
            if ((ch == L',' || ch == L';' || ch == L':') && k + 1 < n && IsPolishLetter(text[k + 1])) {
                spans.push_back({k, 2, DiagnosticCategory::Punctuation,
                                 L"Brak spacji po znaku",
                                 L"Po znaku „" + std::wstring(1, ch) + L"” powinna pojawić się spacja.",
                                 {std::wstring(1, ch) + L" " + std::wstring(1, text[k + 1])}, true});
            }
        }
    }

    // 5. Wielokrotne spacje (styl / formatowanie)
    if (opts.checkStyle) {
        uint32_t k = 0;
        while (k < n) {
            if (text[k] == L' ') {
                uint32_t s = k;
                while (k < n && text[k] == L' ') ++k;
                if (k - s >= 2 && s > 0 && text[s - 1] != L'\n') {
                    spans.push_back({s, k - s, DiagnosticCategory::Style,
                                     L"Wielokrotna spacja",
                                     L"Wykryto nadmiarowe spacje obok siebie.",
                                     {L" "}, true});
                }
            } else {
                ++k;
            }
        }
    }

    // Sortowanie i deduplikacja nakładających się zakresów (priorytet wcześniejszych / ortograficznych)
    std::sort(spans.begin(), spans.end(), [](const DiagnosticSpan& a, const DiagnosticSpan& b) {
        if (a.start != b.start) return a.start < b.start;
        return static_cast<uint8_t>(a.category) < static_cast<uint8_t>(b.category);
    });

    std::vector<DiagnosticSpan> deduped;
    deduped.reserve(spans.size());
    for (auto& sp : spans) {
        bool clash = false;
        for (const auto& existing : deduped) {
            if (Overlaps(existing.start, existing.length, sp.start, sp.length)) {
                clash = true;
                break;
            }
        }
        if (!clash) deduped.push_back(std::move(sp));
    }

    return deduped;
}

} // namespace korektabar
