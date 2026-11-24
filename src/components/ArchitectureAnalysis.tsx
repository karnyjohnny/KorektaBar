import React from 'react';

export const ArchitectureAnalysis: React.FC = () => {
  return (
    <section id="analiza" className="scroll-mt-20 space-y-8">
      <div>
        <h2 className="text-xl font-semibold tracking-tight text-[#ECEFF4]">
          Analiza techniczna i architektura pod Intel Celeron N3350
        </h2>
        <p className="mt-1 text-sm text-[#94A3B8]">
          Zestawienie decyzji inżynierskich zapewniających czas pojawienia okna poniżej 15 ms i zużycie pamięci znacznie poniżej progu 100 MB RAM.
        </p>
      </div>

      {/* Porównanie decyzji architektonicznych */}
      <div className="grid gap-6 lg:grid-cols-2">
        <div className="rounded-xl border border-[#252A34] bg-[#14161A] p-5">
          <h3 className="text-sm font-semibold text-[#ECEFF4]">
            1. Kontrolka edycji: Direct2D / DirectWrite vs RichEdit
          </h3>
          <p className="mt-2 text-xs leading-relaxed text-[#94A3B8]">
            Własny edytor na <code className="text-[#CBD5E1]">IDWriteTextLayout</code> i Direct2D wygrywa z{' '}
            <code className="text-[#CBD5E1]">Msftedit.dll</code> pełną kontrolą nad motywem Dark Mode, brakiem haków suwaków systemowych oraz rysowaniem wielokolorowych falistych podkreśleń w jednym przebiegu renderera bez zaśmiecania stosu Undo/Redo zmianami formatowania RTF.
          </p>
          <div className="mt-4 overflow-x-auto">
            <table className="w-full text-left text-xs">
              <thead>
                <tr className="border-b border-[#252A34] text-[#64748B]">
                  <th className="py-2 pr-3 font-medium">Kryterium</th>
                  <th className="py-2 pr-3 font-medium text-[#38BDF8]">Direct2D + DirectWrite</th>
                  <th className="py-2 font-medium">RichEdit (Msftedit)</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-[#1E232D] text-[#CBD5E1]">
                <tr>
                  <td className="py-2 pr-3 text-[#94A3B8]">Ślad RAM</td>
                  <td className="py-2 pr-3 font-mono">12–20 MB</td>
                  <td className="py-2 font-mono">14–22 MB</td>
                </tr>
                <tr>
                  <td className="py-2 pr-3 text-[#94A3B8]">Podkreślenia błędów</td>
                  <td className="py-2 pr-3">HitTestTextRange + fale RGB</td>
                  <td className="py-2">Ograniczone CHARFORMAT2</td>
                </tr>
                <tr>
                  <td className="py-2 pr-3 text-[#94A3B8]">Dark Mode</td>
                  <td className="py-2 pr-3">Natywny 100%</td>
                  <td className="py-2">Wymaga obejść uxtheme</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>

        <div className="rounded-xl border border-[#252A34] bg-[#14161A] p-5">
          <h3 className="text-sm font-semibold text-[#ECEFF4]">
            2. Silnik ortografii (pl-PL): ISpellChecker vs Hunspell vs Nuspell
          </h3>
          <p className="mt-2 text-xs leading-relaxed text-[#94A3B8]">
            <code className="text-[#CBD5E1]">Nuspell</code> wymaga biblioteki <code className="text-[#CBD5E1]">ICU</code> (~35 MB DLL), a <code className="text-[#CBD5E1]">Hunspell</code> z dużym słownikiem <code className="text-[#CBD5E1]">pl_PL</code> alokuje ok. 25–40 MB w pamięci procesu. Natywny interfejs COM Windows <code className="text-[#CBD5E1]">ISpellChecker</code> współdzieli słownik systemowy (~6–12 MB), a wbudowana tabela pułapek PL gwarantuje działanie nawet bez doinstalowanego pakietu językowego.
          </p>
          <div className="mt-4 overflow-x-auto">
            <table className="w-full text-left text-xs">
              <thead>
                <tr className="border-b border-[#252A34] text-[#64748B]">
                  <th className="py-2 pr-3 font-medium">Silnik</th>
                  <th className="py-2 pr-3 font-medium">RAM procesu</th>
                  <th className="py-2 font-medium">Zależności MSYS2</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-[#1E232D] text-[#CBD5E1]">
                <tr>
                  <td className="py-2 pr-3 font-medium text-[#38BDF8]">ISpellChecker + Fallback PL</td>
                  <td className="py-2 pr-3 font-mono">~6–12 MB</td>
                  <td className="py-2">Zero (Windows API)</td>
                </tr>
                <tr>
                  <td className="py-2 pr-3">Hunspell (pl_PL.dic)</td>
                  <td className="py-2 pr-3 font-mono">~25–40 MB</td>
                  <td className="py-2">libhunspell + słowniki</td>
                </tr>
                <tr>
                  <td className="py-2 pr-3">Nuspell</td>
                  <td className="py-2 pr-3 font-mono">~30–50 MB</td>
                  <td className="py-2">Wymaga ciężkiego ICU</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>

        <div className="rounded-xl border border-[#252A34] bg-[#14161A] p-5">
          <h3 className="text-sm font-semibold text-[#ECEFF4]">
            3. Gramatyka i interpunkcja w budżecie do 100 MB RAM
          </h3>
          <p className="mt-2 text-xs leading-relaxed text-[#94A3B8]">
            Lokalny serwer <code className="text-[#CBD5E1]">LanguageTool</code> w Javie zużywa 450–850 MB RAM, wielokrotnie przekraczając limit 100 MB na Celeronie N3350. Dlatego KorektaBar implementuje deterministyczny, wieloetapowy analizator regułowy bezpośrednio w C++20 (<code className="text-[#CBD5E1]">&lt; 2 MB</code> pamięci): wykrywanie brakujących i zbędnych przecinków, błędy pisowni łącznej/rozłącznej, pleonazmy, powtórzenia sąsiednie/bliskie oraz liniowy formater <code className="text-[#CBD5E1]">O(N)</code>.
          </p>
        </div>

        <div className="rounded-xl border border-[#252A34] bg-[#14161A] p-5">
          <h3 className="text-sm font-semibold text-[#ECEFF4]">
            4. Pamięć w tle, natychmiastowy start i wątki UTF-16
          </h3>
          <p className="mt-2 text-xs leading-relaxed text-[#94A3B8]">
            Ukryte okno <code className="text-[#CBD5E1]">HWND</code> jest tworzone raz przy starcie do zasobnika. Po 3 sekundach od ukrycia zwalniany jest bufor DXGI oraz wywoływane <code className="text-[#CBD5E1]">SetProcessWorkingSetSizeEx</code>. Komunikacja między wątkiem UI a wątkiem analizy (debounce 300 ms) używa licznika <code className="text-[#CBD5E1]">std::atomic&lt;uint64_t&gt;</code>, <code className="text-[#CBD5E1]">SRWLOCK</code> oraz natywnych indeksów UTF-16 zgodnych z <code className="text-[#CBD5E1]">IDWriteTextLayout</code>.
          </p>
        </div>
      </div>

      {/* Zbiorczy budżet RAM */}
      <div className="rounded-xl border border-[#252A34] bg-[#14161A] p-5">
        <div className="mb-3 flex flex-wrap items-center justify-between gap-2">
          <h3 className="text-sm font-semibold text-[#ECEFF4]">
            Budżet pamięci RAM (limit docelowy: 100 MB)
          </h3>
          <span className="font-mono text-xs text-[#38BDF8] tabular-nums">
            Szacowane zużycie aktywne: ~18–35 MB · W tle: ~5–10 MB
          </span>
        </div>
        <div className="h-3 w-full overflow-hidden rounded-full bg-[#1E232D]">
          <div className="flex h-full w-[35%]">
            <div className="h-full w-[57%] bg-[#3B82F6]" title="Win32 + Direct2D/DirectWrite (~20 MB)" />
            <div className="h-full w-[34%] bg-[#38BDF8]" title="ISpellChecker pl-PL (~12 MB)" />
            <div className="h-full w-[9%] bg-[#FBBF24]" title="Analizator reguł PL (< 2 MB)" />
          </div>
        </div>
        <div className="mt-3 flex flex-wrap gap-6 text-xs text-[#94A3B8] tabular-nums">
          <span className="inline-flex items-center gap-1.5">
            <span className="h-2 w-2 rounded-full bg-[#3B82F6]" />
            Win32 + Direct2D/DirectWrite: 12–20 MB
          </span>
          <span className="inline-flex items-center gap-1.5">
            <span className="h-2 w-2 rounded-full bg-[#38BDF8]" />
            Windows ISpellChecker (pl-PL): 6–12 MB
          </span>
          <span className="inline-flex items-center gap-1.5">
            <span className="h-2 w-2 rounded-full bg-[#FBBF24]" />
            Natywne reguły PL + Formater: &lt; 2 MB
          </span>
        </div>
      </div>
    </section>
  );
};
