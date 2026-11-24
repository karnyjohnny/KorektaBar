import React, { useMemo, useState } from 'react';
import { Check, Copy, FileCode2, Terminal } from 'lucide-react';
import { ICO_ASSET_URL, REPO_FILES, RepoFileEntry } from '../data/repoFiles';

type GroupFilter = 'Wszystkie' | RepoFileEntry['group'];

export const RepoExplorer: React.FC = () => {
  const [group, setGroup] = useState<GroupFilter>('Wszystkie');
  const [selectedPath, setSelectedPath] = useState<string>(REPO_FILES[0].path);
  const [copiedPath, setCopiedPath] = useState<string | null>(null);

  const filteredFiles = useMemo(() => {
    if (group === 'Wszystkie') return REPO_FILES;
    return REPO_FILES.filter((f) => f.group === group);
  }, [group]);

  const activeFile = useMemo(
    () => REPO_FILES.find((f) => f.path === selectedPath) ?? REPO_FILES[0],
    [selectedPath],
  );

  const handleCopy = async (text: string, label: string) => {
    try {
      await navigator.clipboard.writeText(text);
      setCopiedPath(label);
      window.setTimeout(() => setCopiedPath(null), 1800);
    } catch {
      // Ignoruj ograniczenia schowka
    }
  };

  const msysCommands = `pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-python make\npython3 scripts/generate_assets.py\nmake -C native -j$(nproc)`;

  return (
    <div className="space-y-12">
      {/* Przeglądarka plików C++ i GitHub */}
      <section id="kod" className="scroll-mt-20 space-y-4">
        <div className="flex flex-wrap items-center justify-between gap-4">
          <div>
            <h2 className="text-xl font-semibold tracking-tight text-[#ECEFF4]">
              Produkcyjny kod źródłowy C++20 i zasoby repozytorium
            </h2>
            <p className="mt-1 text-sm text-[#94A3B8]">
              Kompletna struktura gotowa do wrzucenia na GitHuba i kompilacji w MSYS2 UCRT64.
            </p>
          </div>

          <div className="flex items-center gap-1 rounded-lg border border-[#252A34] bg-[#14161A] p-1">
            {(['Wszystkie', 'C++ Rdzeń', 'Zasoby i MSYS2', 'GitHub i Skrypty'] as GroupFilter[]).map(
              (g) => (
                <button
                  key={g}
                  type="button"
                  onClick={() => setGroup(g)}
                  className={`rounded-md px-3 py-1.5 text-xs font-medium transition-colors whitespace-nowrap ${
                    group === g
                      ? 'bg-[#2563EB] text-white'
                      : 'text-[#94A3B8] hover:text-[#ECEFF4]'
                  }`}
                >
                  {g}
                </button>
              ),
            )}
          </div>
        </div>

        <div className="grid overflow-hidden rounded-xl border border-[#252A34] bg-[#14161A] lg:grid-cols-[280px_1fr]">
          {/* Lista plików */}
          <div className="max-h-[520px] overflow-y-auto border-b border-[#252A34] bg-[#111318] p-2 lg:border-r lg:border-b-0">
            {filteredFiles.map((file) => {
              const active = file.path === activeFile.path;
              return (
                <button
                  key={file.path}
                  type="button"
                  onClick={() => setSelectedPath(file.path)}
                  className={`flex w-full items-center gap-2 rounded-md px-2.5 py-2 text-left font-mono text-xs transition-colors ${
                    active
                      ? 'bg-[#2563EB]/20 text-[#38BDF8]'
                      : 'text-[#94A3B8] hover:bg-[#1A1E26] hover:text-[#ECEFF4]'
                  }`}
                >
                  <FileCode2 className="h-3.5 w-3.5 shrink-0 opacity-75" />
                  <span className="truncate">{file.path}</span>
                </button>
              );
            })}
          </div>

          {/* Podgląd zawartości pliku */}
          <div className="flex min-w-0 flex-col">
            <div className="flex items-center justify-between border-b border-[#252A34] bg-[#161920] px-4 py-2.5">
              <div className="flex items-center gap-2 min-w-0">
                <span className="truncate font-mono text-xs font-medium text-[#ECEFF4]">
                  {activeFile.path}
                </span>
                <span className="text-xs text-[#64748B] whitespace-nowrap">
                  · {activeFile.content.split('\n').length} linii
                </span>
              </div>
              <button
                type="button"
                onClick={() => handleCopy(activeFile.content, activeFile.path)}
                className="inline-flex items-center gap-1.5 rounded-md border border-[#2A2F3A] bg-[#1D222B] px-2.5 py-1 text-xs font-medium text-[#CBD5E1] transition-colors hover:border-[#3B82F6] hover:text-white whitespace-nowrap shrink-0"
              >
                {copiedPath === activeFile.path ? (
                  <>
                    <Check className="h-3.5 w-3.5 text-[#38BDF8]" />
                    Skopiowano
                  </>
                ) : (
                  <>
                    <Copy className="h-3.5 w-3.5" />
                    Kopiuj plik
                  </>
                )}
              </button>
            </div>

            <pre className="max-h-[472px] overflow-auto p-4 font-mono text-xs leading-relaxed text-[#CBD5E1]">
              <code>{activeFile.content}</code>
            </pre>
          </div>
        </div>
      </section>

      {/* Sekcja budowania MSYS2, ikony oraz banera GitHub */}
      <section id="budowanie" className="scroll-mt-20 grid gap-6 lg:grid-cols-[1.3fr_1fr]">
        <div className="rounded-xl border border-[#252A34] bg-[#14161A] p-5">
          <div className="mb-3 flex items-center justify-between">
            <div className="flex items-center gap-2">
              <Terminal className="h-4 w-4 text-[#38BDF8]" />
              <h3 className="text-sm font-semibold text-[#ECEFF4]">
                Budowanie binarki w MSYS2 (UCRT64)
              </h3>
            </div>
            <button
              type="button"
              onClick={() => handleCopy(msysCommands, 'msys')}
              className="inline-flex items-center gap-1.5 rounded-md border border-[#2A2F3A] bg-[#1D222B] px-2.5 py-1 text-xs font-medium text-[#CBD5E1] hover:border-[#3B82F6] hover:text-white"
            >
              {copiedPath === 'msys' ? 'Skopiowano' : 'Kopiuj komendy'}
            </button>
          </div>
          <p className="mb-3 text-xs leading-relaxed text-[#94A3B8]">
            Kompilacja produkuje pojedynczy plik <code className="text-[#ECEFF4]">native/build/KorektaBar.exe</code> ze statycznie dołączoną biblioteką standardową C++ (bez wymaganych zewnętrznych DLL). Workflow <code className="text-[#ECEFF4]">.github/workflows/release.yml</code> wykonuje te same kroki automatycznie przy wypchnięciu taga <code className="text-[#ECEFF4]">v*</code>.
          </p>
          <pre className="overflow-x-auto rounded-lg border border-[#232833] bg-[#0D0F13] p-3.5 font-mono text-xs leading-relaxed text-[#38BDF8]">
            <code>{msysCommands}</code>
          </pre>
        </div>

        <div className="rounded-xl border border-[#252A34] bg-[#14161A] p-5">
          <h3 className="text-sm font-semibold text-[#ECEFF4]">
            Wygenerowane zasoby graficzne (.ico i baner)
          </h3>
          <p className="mt-1.5 text-xs text-[#94A3B8]">
            Wielowarstwowa ikona Windows (<code className="text-[#CBD5E1]">16×16, 32×32, 48×48, 256×256</code>) oraz wektorowy baner Dark Mode dla <code className="text-[#CBD5E1]">README.md</code>.
          </p>

          <div className="mt-4 flex items-center gap-4">
            <div className="flex h-16 w-16 shrink-0 items-center justify-center rounded-lg border border-[#2A2F3A] bg-[#0D0F13]">
              <img
                src={ICO_ASSET_URL}
                alt="Ikona aplikacji KorektaBar.ico"
                className="h-12 w-12 object-contain"
                referrerPolicy="no-referrer"
              />
            </div>
            <div className="min-w-0 text-xs">
              <div className="font-mono font-medium text-[#ECEFF4]">assets/KorektaBar.ico</div>
              <div className="mt-0.5 text-[#94A3B8]">
                Podłączona przez <code className="text-[#CBD5E1]">KorektaBar.rc</code> do zasobnika systemowego i paska tytułu.
              </div>
              <a
                href={ICO_ASSET_URL}
                download="KorektaBar.ico"
                className="mt-1.5 inline-block font-medium text-[#38BDF8] hover:underline"
              >
                Pobierz KorektaBar.ico
              </a>
            </div>
          </div>
        </div>
      </section>
    </div>
  );
};
