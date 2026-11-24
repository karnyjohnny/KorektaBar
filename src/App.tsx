import React, { useState } from 'react';
import { Download } from 'lucide-react';
import { KorektaBarSimulator } from './components/KorektaBarSimulator';
import { ArchitectureAnalysis } from './components/ArchitectureAnalysis';
import { RepoExplorer } from './components/RepoExplorer';
import { ICO_ASSET_URL, REPO_FILES } from './data/repoFiles';
import { ZipInputFile, buildZipBlob } from './lib/zipExport';

export default function App() {
  const [downloading, setDownloading] = useState(false);

  const handleDownloadZip = async () => {
    setDownloading(true);
    try {
      const files: ZipInputFile[] = REPO_FILES.map((f) => ({
        path: f.path,
        content: f.content,
      }));

      try {
        const resp = await fetch(ICO_ASSET_URL);
        if (resp.ok) {
          const buf = new Uint8Array(await resp.arrayBuffer());
          files.push({ path: 'assets/KorektaBar.ico', content: buf });
        }
      } catch {
        // Jeśli pobranie pliku binarnego nie powiedzie się, paczka nadal zawiera scripts/generate_assets.py
      }

      const blob = buildZipBlob(files);
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = 'KorektaBar-repo.zip';
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    } finally {
      setDownloading(false);
    }
  };

  return (
    <div className="min-h-screen bg-[#0E1014] text-[#ECEFF4]">
      {/* Kontrakt górnego paska: 3 strefy (Wordmark — 4 linki — 1 akcja) */}
      <header className="sticky top-0 z-30 border-b border-[#1E232D] bg-[#0E1014]/90 backdrop-blur-md">
        <div className="mx-auto flex max-w-6xl items-center justify-between gap-8 px-6 py-4">
          <a
            href="#symulator"
            className="text-lg font-bold tracking-tight text-[#ECEFF4] whitespace-nowrap shrink-0"
          >
            KorektaBar
          </a>

          <nav className="hidden items-center gap-6 text-sm font-medium text-[#94A3B8] md:flex">
            <a
              href="#symulator"
              className="transition-colors hover:text-[#ECEFF4] whitespace-nowrap shrink-0"
            >
              Symulator
            </a>
            <a
              href="#analiza"
              className="transition-colors hover:text-[#ECEFF4] whitespace-nowrap shrink-0"
            >
              Analiza
            </a>
            <a
              href="#kod"
              className="transition-colors hover:text-[#ECEFF4] whitespace-nowrap shrink-0"
            >
              Kod C++
            </a>
            <a
              href="#budowanie"
              className="transition-colors hover:text-[#ECEFF4] whitespace-nowrap shrink-0"
            >
              Budowanie
            </a>
          </nav>

          <div className="flex items-center shrink-0">
            <button
              type="button"
              onClick={handleDownloadZip}
              disabled={downloading}
              className="inline-flex items-center gap-2 rounded-lg bg-[#2563EB] px-4 py-2 text-xs font-semibold text-white transition-colors hover:bg-[#3B82F6] disabled:opacity-60 whitespace-nowrap shrink-0"
            >
              <Download className="h-3.5 w-3.5" />
              {downloading ? 'Pakowanie…' : 'Pobierz projekt (.zip)'}
            </button>
          </div>
        </div>
      </header>

      <main className="mx-auto max-w-6xl space-y-14 px-6 py-10">
        {/* Zwięzłe wprowadzenie nad symulatorem */}
        <div className="flex flex-col gap-3 md:flex-row md:items-end md:justify-between">
          <div>
            <h1 className="max-w-2xl text-3xl font-bold tracking-tight text-[#ECEFF4] sm:text-4xl [text-wrap:balance]">
              Natywny korektor i formater języka polskiego w C++20
            </h1>
            <p className="mt-2 max-w-2xl text-sm leading-relaxed text-[#94A3B8]">
              Architektura Win32 API + Direct2D/DirectWrite dla Windows 10 i Intel Celeron N3350.
              Poniżej przetestujesz logikę korekty na żywo, sprawdzisz analizę techniczną i pobierzesz gotowe repozytorium pod MSYS2 oraz GitHub Actions.
            </p>
          </div>
          <div className="flex items-center gap-2 text-xs text-[#64748B] whitespace-nowrap">
            <span>Windows 10</span>
            <span aria-hidden="true">·</span>
            <span>Direct2D / DirectWrite</span>
            <span aria-hidden="true">·</span>
            <span className="font-mono text-[#94A3B8] tabular-nums">&lt; 35 MB RAM</span>
          </div>
        </div>

        <KorektaBarSimulator />
        <ArchitectureAnalysis />
        <RepoExplorer />
      </main>

      <footer className="border-t border-[#1E232D] py-6 text-center text-xs text-[#64748B]">
        KorektaBar · Natywny projekt C++20 dla Windows 10 (MSYS2 UCRT64 + GitHub Actions Releases)
      </footer>
    </div>
  );
}
