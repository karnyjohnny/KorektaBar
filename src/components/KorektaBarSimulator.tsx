import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  CheckCheck,
  Copy,
  EyeOff,
  RotateCcw,
  Settings2,
  SlidersHorizontal,
  Wand2,
} from 'lucide-react';
import {
  AnalysisOptions,
  DiagnosticSpan,
  FormatterOptions,
  analyzePolishText,
  applyAutoFixAll,
  formatPolishText,
} from '../lib/polishEngine';

const SAMPLE_ERRORS =
  'Wogóle myślę że napewno  jest jest okres czasu ,aby wziąść książkę i przeczytać ją póżniej.\n' +
  'Kiedy poszłem spowrotem do domu ,zobaczyłem zolty zeszyt i,pomyślałem że to fakt autentyczny.';

const SAMPLE_MESSY =
  '   cześć   wszystkim ,dzisiaj   testujemy   nowy formater tekstu .  \n\n\n\n' +
  '  usuwa on   nadmiarowe spacje ,poprawia   interpunkcje   i wielkie litery!czy to działa ?tak ,działa .   ';

interface ContextMenuState {
  x: number;
  y: number;
  span: DiagnosticSpan;
}

interface HotkeyConfig {
  ctrl: boolean;
  alt: boolean;
  shift: boolean;
  key: string;
  label: string;
}

export const KorektaBarSimulator: React.FC = () => {
  const [text, setText] = useState<string>(SAMPLE_ERRORS);
  const [undoStack, setUndoStack] = useState<string[]>([]);
  const [diagnostics, setDiagnostics] = useState<DiagnosticSpan[]>(() =>
    analyzePolishText(SAMPLE_ERRORS, { enableSpelling: true, enablePunctuation: true, enableStyle: true }, new Set()),
  );
  const [ignoredWords, setIgnoredWords] = useState<Set<string>>(() => new Set());
  const [isVisible, setIsVisible] = useState<boolean>(true);
  const [isAnalyzing, setIsAnalyzing] = useState<boolean>(false);
  const [toast, setToast] = useState<string | null>(null);
  const [contextMenu, setContextMenu] = useState<ContextMenuState | null>(null);
  const [settingsOpen, setSettingsOpen] = useState<boolean>(false);

  const [analysisOpts, setAnalysisOpts] = useState<AnalysisOptions>({
    enableSpelling: true,
    enablePunctuation: true,
    enableStyle: true,
  });

  const [formatterOpts, setFormatterOpts] = useState<FormatterOptions>({
    collapseSpaces: true,
    fixPunctuationSpacing: true,
    normalizeParagraphs: true,
    capitalizeSentences: true,
  });

  const [hotkey, setHotkey] = useState<HotkeyConfig>({
    ctrl: true,
    alt: false,
    shift: false,
    key: ' ',
    label: 'Ctrl + Space',
  });
  const [capturingHotkey, setCapturingHotkey] = useState<boolean>(false);

  const textareaRef = useRef<HTMLTextAreaElement | null>(null);
  const mirrorRef = useRef<HTMLDivElement | null>(null);
  const toastTimerRef = useRef<number | null>(null);

  const showNotice = useCallback((msg: string) => {
    setToast(msg);
    if (toastTimerRef.current) window.clearTimeout(toastTimerRef.current);
    toastTimerRef.current = window.setTimeout(() => setToast(null), 2200);
  }, []);

  // Symulacja wątku tła z 280 ms debounce
  useEffect(() => {
    setIsAnalyzing(true);
    const timer = window.setTimeout(() => {
      const next = analyzePolishText(text, analysisOpts, ignoredWords);
      setDiagnostics(next);
      setIsAnalyzing(false);
    }, 280);
    return () => window.clearTimeout(timer);
  }, [text, analysisOpts, ignoredWords]);

  // Globalny skrót Ctrl + Space (lub skonfigurowany w Opcjach) w oknie podglądu
  useEffect(() => {
    const handleGlobalKeyDown = (e: KeyboardEvent) => {
      if (capturingHotkey) return;
      const keyMatch = e.key === hotkey.key || (hotkey.key === ' ' && e.code === 'Space');
      if (
        keyMatch &&
        e.ctrlKey === hotkey.ctrl &&
        e.altKey === hotkey.alt &&
        e.shiftKey === hotkey.shift
      ) {
        e.preventDefault();
        setIsVisible((prev) => {
          const next = !prev;
          if (next) {
            window.setTimeout(() => textareaRef.current?.focus(), 20);
          }
          return next;
        });
      }
      if (e.key === 'Escape' && contextMenu) {
        setContextMenu(null);
      }
    };
    window.addEventListener('keydown', handleGlobalKeyDown);
    return () => window.removeEventListener('keydown', handleGlobalKeyDown);
  }, [hotkey, capturingHotkey, contextMenu]);

  // Zamknięcie menu kontekstowego po kliknięciu poza nim
  useEffect(() => {
    if (!contextMenu) return;
    const close = () => setContextMenu(null);
    window.addEventListener('click', close);
    return () => window.removeEventListener('click', close);
  }, [contextMenu]);

  const commitTextChange = useCallback(
    (nextText: string) => {
      if (nextText === text) return;
      setUndoStack((prev) => [...prev.slice(-99), text]);
      setText(nextText);
    },
    [text],
  );

  const handleUndo = useCallback(() => {
    setUndoStack((prev) => {
      if (prev.length === 0) return prev;
      const last = prev[prev.length - 1];
      setText(last);
      return prev.slice(0, -1);
    });
  }, []);

  const handleAutoFixAll = () => {
    const { text: fixed, fixedCount } = applyAutoFixAll(text, diagnostics);
    if (fixedCount > 0) {
      commitTextChange(fixed);
      showNotice(`Zastosowano ${fixedCount} poprawek`);
    } else {
      showNotice('Brak automatycznych poprawek do zastosowania');
    }
  };

  const handleFormat = () => {
    const formatted = formatPolishText(text, formatterOpts);
    if (formatted !== text) {
      commitTextChange(formatted);
      showNotice('Tekst został sformatowany');
    } else {
      showNotice('Tekst ma już poprawne formatowanie');
    }
  };

  const handleCopy = async () => {
    try {
      await navigator.clipboard.writeText(text);
      showNotice('Skopiowano do schowka');
    } catch {
      showNotice('Gotowe do skopiowania (Ctrl+C)');
    }
  };

  const applySingleFix = (span: DiagnosticSpan, replacement: string) => {
    const updated = text.slice(0, span.start) + replacement + text.slice(span.start + span.length);
    commitTextChange(updated);
    setContextMenu(null);
  };

  const ignoreWord = (span: DiagnosticSpan) => {
    const fragment = text.slice(span.start, span.start + span.length).toLocaleLowerCase('pl-PL');
    setIgnoredWords((prev) => {
      const next = new Set(prev);
      next.add(fragment);
      return next;
    });
    setContextMenu(null);
  };

  const syncScroll = () => {
    if (textareaRef.current && mirrorRef.current) {
      mirrorRef.current.scrollTop = textareaRef.current.scrollTop;
    }
  };

  // Precyzyjne wykrywanie klikniętego błędu pod PPM
  const handleTextareaContextMenu = (e: React.MouseEvent<HTMLTextAreaElement>) => {
    const ta = textareaRef.current;
    const mirror = mirrorRef.current;
    if (!ta || !mirror || diagnostics.length === 0) return;

    let hitSpan: DiagnosticSpan | undefined;

    // Próba odczytu elementu span w warstwie lustrzanej pod współrzędnymi kliknięcia
    const prevPointer = mirror.style.pointerEvents;
    const prevZ = mirror.style.zIndex;
    mirror.style.pointerEvents = 'auto';
    mirror.style.zIndex = '20';
    const el = document.elementFromPoint(e.clientX, e.clientY) as HTMLElement | null;
    mirror.style.pointerEvents = prevPointer;
    mirror.style.zIndex = prevZ;

    const spanId = el?.dataset?.spanId;
    if (spanId) {
      hitSpan = diagnostics.find((d) => d.id === spanId);
    }

    if (!hitSpan) {
      const caret = ta.selectionStart ?? 0;
      hitSpan = diagnostics.find((d) => caret >= d.start && caret <= d.start + d.length);
    }

    if (hitSpan) {
      e.preventDefault();
      setContextMenu({
        x: e.clientX,
        y: e.clientY,
        span: hitSpan,
      });
    }
  };

  const handleHotkeyCapture = (e: React.KeyboardEvent<HTMLInputElement>) => {
    e.preventDefault();
    if (['Control', 'Shift', 'Alt', 'Meta'].includes(e.key)) return;
    const ctrl = e.ctrlKey || (!e.altKey && !e.shiftKey);
    const alt = e.altKey;
    const shift = e.shiftKey;
    const keyLabel = e.key === ' ' ? 'Space' : e.key.length === 1 ? e.key.toUpperCase() : e.key;
    const parts: string[] = [];
    if (ctrl) parts.push('Ctrl');
    if (alt) parts.push('Alt');
    if (shift) parts.push('Shift');
    parts.push(keyLabel);

    setHotkey({ ctrl, alt, shift, key: e.key, label: parts.join(' + ') });
    setCapturingHotkey(false);
    showNotice(`Zmieniono skrót na ${parts.join(' + ')}`);
  };

  const counts = useMemo(() => {
    let spelling = 0;
    let punctuation = 0;
    let style = 0;
    for (const d of diagnostics) {
      if (d.category === 'spelling') spelling++;
      else if (d.category === 'punctuation') punctuation++;
      else style++;
    }
    return { spelling, punctuation, style };
  }, [diagnostics]);

  // Segmenty tekstu do warstwy podkreśleń
  const highlightedNodes = useMemo(() => {
    if (diagnostics.length === 0) return <span>{text || ' '}</span>;
    const nodes: React.ReactNode[] = [];
    let cursor = 0;

    for (const sp of diagnostics) {
      if (sp.start > cursor) {
        nodes.push(<span key={`t-${cursor}`}>{text.slice(cursor, sp.start)}</span>);
      }
      const cls =
        sp.category === 'spelling'
          ? 'squiggle-spelling'
          : sp.category === 'punctuation'
            ? 'squiggle-punctuation'
            : 'squiggle-style';

      nodes.push(
        <span key={sp.id} data-span-id={sp.id} className={cls}>
          {text.slice(sp.start, sp.start + sp.length)}
        </span>,
      );
      cursor = sp.start + sp.length;
    }
    if (cursor < text.length) {
      nodes.push(<span key={`t-end`}>{text.slice(cursor)}</span>);
    }
    if (text.endsWith('\n')) {
      nodes.push(<span key="trail-nl">&nbsp;</span>);
    }
    return nodes;
  }, [text, diagnostics]);

  return (
    <section id="symulator" className="scroll-mt-20">
      <div className="mb-4 flex flex-wrap items-center justify-between gap-4">
        <div>
          <h2 className="text-xl font-semibold tracking-tight text-[#ECEFF4]">
            Interaktywny symulator okna KorektaBar
          </h2>
          <p className="mt-1 text-sm text-[#94A3B8]">
            Wpisuj tekst, kliknij prawym przyciskiem myszy na podkreślonym słowie lub wciśnij{' '}
            <kbd className="rounded border border-[#2D3545] bg-[#181B22] px-1.5 py-0.5 font-mono text-xs text-[#ECEFF4]">
              {hotkey.label}
            </kbd>{' '}
            aby przywołać lub ukryć pasek nad paskiem zadań.
          </p>
        </div>

        <div className="flex flex-wrap items-center gap-2">
          <button
            type="button"
            onClick={() => {
              commitTextChange(SAMPLE_ERRORS);
              setIgnoredWords(new Set());
            }}
            className="rounded-md border border-[#2A2F3A] bg-[#161920] px-3 py-1.5 text-xs font-medium text-[#CBD5E1] transition-colors hover:border-[#3B82F6]/60 hover:text-white whitespace-nowrap"
          >
            Przykład z błędami
          </button>
          <button
            type="button"
            onClick={() => {
              commitTextChange(SAMPLE_MESSY);
              setIgnoredWords(new Set());
            }}
            className="rounded-md border border-[#2A2F3A] bg-[#161920] px-3 py-1.5 text-xs font-medium text-[#CBD5E1] transition-colors hover:border-[#3B82F6]/60 hover:text-white whitespace-nowrap"
          >
            Niechlujny tekst
          </button>
          <button
            type="button"
            onClick={() => commitTextChange('')}
            className="rounded-md border border-[#2A2F3A] bg-[#161920] px-3 py-1.5 text-xs font-medium text-[#94A3B8] transition-colors hover:border-[#475569] hover:text-white whitespace-nowrap"
          >
            Wyczyść
          </button>
        </div>
      </div>

      {/* Symulowany pulpit Windows 10 z pozycjonowaniem 10px nad paskiem zadań */}
      <div className="relative overflow-hidden rounded-xl border border-[#252A34] bg-[#0B0D11]">
        {/* Tło robocze pulpitu */}
        <div
          className="relative flex min-h-[360px] flex-col justify-end px-4 pt-8 pb-[10px]"
          style={{
            backgroundImage:
              'radial-gradient(circle at 75% 20%, rgba(59, 130, 246, 0.09), transparent 55%), linear-gradient(180deg, #0D1016 0%, #0A0C10 100%)',
          }}
        >
          {/* Oznaczenie pozycji w tle pulpitu gdy okno ukryte */}
          {!isVisible && (
            <div className="mx-auto my-auto flex flex-col items-center gap-3 text-center">
              <p className="text-sm text-[#94A3B8]">
                Okno KorektaBar zostało schowane do zasobnika systemowego (pamięć w tle ~6 MB RAM).
              </p>
              <button
                type="button"
                onClick={() => setIsVisible(true)}
                className="rounded-lg bg-[#2563EB] px-4 py-2 text-xs font-semibold text-white transition-colors hover:bg-[#3B82F6]"
              >
                Przywołaj okno ({hotkey.label})
              </button>
            </div>
          )}

          {/* Okno KorektaBar o proporcjach klawiatury ekranowej */}
          {isVisible && (
            <div className="mx-auto w-full max-w-[920px] rounded-[10px] border border-[#2D3442] bg-[#14161A] shadow-2xl">
              {/* Pasek narzędzi */}
              <div className="flex flex-wrap items-center justify-between gap-2 border-b border-[#232833] px-3.5 py-2.5">
                <div className="flex items-center gap-3">
                  <span className="text-sm font-semibold tracking-tight text-[#ECEFF4]">
                    KorektaBar
                  </span>
                  <span className="font-mono text-xs text-[#64748B]">{hotkey.label}</span>
                  {toast && (
                    <span className="text-xs font-medium text-[#38BDF8]">{toast}</span>
                  )}
                </div>

                <div className="flex flex-wrap items-center gap-1.5">
                  <button
                    type="button"
                    onClick={handleAutoFixAll}
                    className="inline-flex items-center gap-1.5 rounded-md bg-[#2563EB] px-3 py-1.5 text-xs font-semibold text-white transition-colors hover:bg-[#3B82F6] whitespace-nowrap shrink-0"
                  >
                    <CheckCheck className="h-3.5 w-3.5" />
                    Auto-popraw wszystko
                  </button>

                  <button
                    type="button"
                    onClick={handleFormat}
                    className="inline-flex items-center gap-1.5 rounded-md border border-[#2B313E] bg-[#1D222B] px-2.5 py-1.5 text-xs font-medium text-[#E2E8F0] transition-colors hover:bg-[#262C38] whitespace-nowrap shrink-0"
                  >
                    <Wand2 className="h-3.5 w-3.5 text-[#38BDF8]" />
                    Formatuj tekst
                  </button>

                  <button
                    type="button"
                    onClick={handleCopy}
                    className="inline-flex items-center gap-1.5 rounded-md border border-[#2B313E] bg-[#1D222B] px-2.5 py-1.5 text-xs font-medium text-[#E2E8F0] transition-colors hover:bg-[#262C38] whitespace-nowrap shrink-0"
                  >
                    <Copy className="h-3.5 w-3.5 text-[#94A3B8]" />
                    Kopiuj
                  </button>

                  <button
                    type="button"
                    onClick={handleUndo}
                    disabled={undoStack.length === 0}
                    className="inline-flex items-center gap-1.5 rounded-md border border-[#2B313E] bg-[#1D222B] px-2.5 py-1.5 text-xs font-medium text-[#E2E8F0] transition-colors hover:bg-[#262C38] disabled:opacity-40 whitespace-nowrap shrink-0"
                  >
                    <RotateCcw className="h-3.5 w-3.5 text-[#94A3B8]" />
                    Cofnij
                  </button>

                  <button
                    type="button"
                    onClick={() => setSettingsOpen(true)}
                    className="inline-flex items-center gap-1.5 rounded-md border border-[#2B313E] bg-[#1D222B] px-2.5 py-1.5 text-xs font-medium text-[#E2E8F0] transition-colors hover:bg-[#262C38] whitespace-nowrap shrink-0"
                  >
                    <Settings2 className="h-3.5 w-3.5 text-[#94A3B8]" />
                    Opcje
                  </button>

                  <button
                    type="button"
                    onClick={() => setIsVisible(false)}
                    className="inline-flex items-center gap-1 rounded-md border border-[#2B313E] bg-[#1D222B] px-2.5 py-1.5 text-xs font-medium text-[#94A3B8] transition-colors hover:bg-[#262C38] hover:text-white whitespace-nowrap shrink-0"
                  >
                    <EyeOff className="h-3.5 w-3.5" />
                    Ukryj
                  </button>
                </div>
              </div>

              {/* Obszar edycji DirectWrite (z lustrzaną warstwą podkreśleń) */}
              <div className="p-3.5 pb-2">
                <div className="relative h-[168px] w-full overflow-hidden rounded-lg border border-[#2A2F3A] bg-[#1B1E24] focus-within:border-[#3B82F6]">
                  <div
                    ref={mirrorRef}
                    aria-hidden="true"
                    className="pointer-events-none absolute inset-0 overflow-y-auto whitespace-pre-wrap break-words px-3.5 py-3 text-[15px] leading-[1.6] text-transparent"
                  >
                    {highlightedNodes}
                  </div>

                  <textarea
                    ref={textareaRef}
                    value={text}
                    onChange={(e) => commitTextChange(e.target.value)}
                    onScroll={syncScroll}
                    onContextMenu={handleTextareaContextMenu}
                    spellCheck={false}
                    placeholder="Wpisz lub wklej polski tekst. Prawym przyciskiem myszy na podkreślonym słowie otworzysz poprawki…"
                    className="relative z-10 h-full w-full resize-none bg-transparent px-3.5 py-3 text-[15px] leading-[1.6] text-[#ECEFF4] placeholder-[#64748B] focus:outline-none"
                  />
                </div>
              </div>

              {/* Stopka stanu (z etykietami tekstowymi przy każdym kolorze) */}
              <div className="flex flex-wrap items-center justify-between gap-2 px-3.5 pb-2.5 pt-0.5 text-xs text-[#94A3B8]">
                <div className="flex flex-wrap items-center gap-4 tabular-nums">
                  <span className="inline-flex items-center gap-1.5">
                    <span className="h-2 w-2 rounded-full bg-[#F87171]" />
                    Ortografia: <strong className="font-medium text-[#ECEFF4]">{counts.spelling}</strong>
                  </span>
                  <span className="inline-flex items-center gap-1.5">
                    <span className="h-2 w-2 rounded-full bg-[#38BDF8]" />
                    Interpunkcja: <strong className="font-medium text-[#ECEFF4]">{counts.punctuation}</strong>
                  </span>
                  <span className="inline-flex items-center gap-1.5">
                    <span className="h-2 w-2 rounded-full bg-[#FBBF24]" />
                    Styl: <strong className="font-medium text-[#ECEFF4]">{counts.style}</strong>
                  </span>
                </div>

                <div className="flex items-center gap-2 font-mono text-[11px] text-[#64748B] tabular-nums">
                  <span>{isAnalyzing ? 'Analiza w tle…' : 'Gotowy'}</span>
                  <span aria-hidden="true">·</span>
                  <span>PPM na błędzie: sugestie</span>
                  <span aria-hidden="true">·</span>
                  <span>{text.length} zn.</span>
                </div>
              </div>
            </div>
          )}
        </div>

        {/* Pasek zadań Windows 10 (dokładnie 10 px poniżej okna KorektaBar) */}
        <div className="flex h-10 items-center justify-between border-t border-[#1F242E] bg-[#090B0F] px-3 text-xs text-[#94A3B8]">
          <div className="flex items-center gap-3">
            <div className="grid grid-cols-2 gap-0.5 opacity-80">
              <span className="h-1.5 w-1.5 bg-[#38BDF8]" />
              <span className="h-1.5 w-1.5 bg-[#38BDF8]" />
              <span className="h-1.5 w-1.5 bg-[#38BDF8]" />
              <span className="h-1.5 w-1.5 bg-[#38BDF8]" />
            </div>
            <span className="text-[#64748B]">Windows 10 Taskbar</span>
            <span className="font-mono text-[11px] text-[#475569]">odstęp okna: 10 px</span>
          </div>

          <div className="flex items-center gap-3">
            <button
              type="button"
              onClick={() => setIsVisible((v) => !v)}
              title="Zasobnik systemowy KorektaBar (kliknij, aby pokazać/ukryć)"
              className="flex items-center gap-2 rounded px-2 py-1 text-xs text-[#CBD5E1] transition-colors hover:bg-[#1B2029]"
            >
              <span className="flex h-4 w-4 items-center justify-center rounded bg-[#181C24] border border-[#2E3646] text-[10px] font-bold text-[#38BDF8]">
                K
              </span>
              <span>Tray</span>
            </button>
            <span className="font-mono text-[11px] text-[#64748B] tabular-nums">12:00</span>
          </div>
        </div>
      </div>

      {/* Lista wykrytych poprawek (wygodne szybkie kliknięcie także bez myszy PPM) */}
      {diagnostics.length > 0 && (
        <div className="mt-4 rounded-lg border border-[#252A34] bg-[#12151B] p-3.5">
          <div className="mb-2.5 flex items-center justify-between">
            <span className="text-xs font-semibold text-[#CBD5E1]">
              Wykryte miejsca do korekty ({diagnostics.length})
            </span>
            <span className="text-xs text-[#64748B]">
              Kliknij propozycję, aby podmienić fragment lub kliknij PPM w edytorze
            </span>
          </div>
          <div className="grid gap-2 sm:grid-cols-2 lg:grid-cols-3">
            {diagnostics.slice(0, 9).map((sp) => {
              const badgeColor =
                sp.category === 'spelling'
                  ? 'text-[#F87171]'
                  : sp.category === 'punctuation'
                    ? 'text-[#38BDF8]'
                    : 'text-[#FBBF24]';
              const fragment = text.slice(sp.start, sp.start + sp.length);
              return (
                <div
                  key={sp.id}
                  className="flex items-center justify-between gap-2 rounded-md border border-[#222732] bg-[#171B22] px-3 py-2 text-xs"
                >
                  <div className="min-w-0">
                    <div className="flex items-center gap-1.5">
                      <span className={`font-semibold ${badgeColor}`}>{sp.title}</span>
                    </div>
                    <p className="mt-0.5 truncate text-[#94A3B8]">
                      „{fragment}”
                    </p>
                  </div>
                  {sp.suggestions.length > 0 ? (
                    <button
                      type="button"
                      onClick={() => applySingleFix(sp, sp.suggestions[0])}
                      className="rounded border border-[#2E3646] bg-[#202632] px-2 py-1 font-medium text-[#ECEFF4] transition-colors hover:border-[#3B82F6] hover:bg-[#2563EB] hover:text-white whitespace-nowrap shrink-0"
                    >
                      {sp.suggestions[0] === '' ? 'Usuń' : `→ ${sp.suggestions[0]}`}
                    </button>
                  ) : (
                    <button
                      type="button"
                      onClick={() => ignoreWord(sp)}
                      className="rounded border border-[#2E3646] px-2 py-1 text-[#94A3B8] hover:text-white whitespace-nowrap shrink-0"
                    >
                      Ignoruj
                    </button>
                  )}
                </div>
              );
            })}
          </div>
        </div>
      )}

      {/* Natywne menu kontekstowe PPM */}
      {contextMenu && (
        <div
          onClick={(e) => e.stopPropagation()}
          style={{
            left: Math.min(contextMenu.x, window.innerWidth - 260),
            top: Math.min(contextMenu.y, window.innerHeight - 210),
          }}
          className="fixed z-50 w-64 rounded-lg border border-[#343C4C] bg-[#1B1F27] p-1.5 shadow-2xl"
        >
          <div className="border-b border-[#29303D] px-2.5 py-2">
            <div className="text-xs font-semibold text-[#ECEFF4]">{contextMenu.span.title}</div>
            <div className="mt-0.5 text-[11px] leading-snug text-[#94A3B8]">
              {contextMenu.span.explanation}
            </div>
          </div>
          <div className="py-1">
            {contextMenu.span.suggestions.length > 0 ? (
              contextMenu.span.suggestions.map((sugg, idx) => (
                <button
                  key={idx}
                  type="button"
                  onClick={() => applySingleFix(contextMenu.span, sugg)}
                  className="flex w-full items-center justify-between rounded px-2.5 py-1.5 text-left text-xs font-medium text-[#ECEFF4] transition-colors hover:bg-[#2563EB]"
                >
                  <span>{sugg === '' ? 'Usuń zbędny znak' : `Zamień na: „${sugg}”`}</span>
                </button>
              ))
            ) : (
              <div className="px-2.5 py-1.5 text-xs text-[#64748B]">
                Brak automatycznej propozycji
              </div>
            )}
          </div>
          <div className="border-t border-[#29303D] pt-1">
            <button
              type="button"
              onClick={() => ignoreWord(contextMenu.span)}
              className="w-full rounded px-2.5 py-1.5 text-left text-xs text-[#94A3B8] transition-colors hover:bg-[#262C38] hover:text-white"
            >
              Ignoruj w tej sesji
            </button>
          </div>
        </div>
      )}

      {/* Okno Opcje (odzwierciedla SettingsDialog.cpp) */}
      {settingsOpen && (
        <div
          className="fixed inset-0 z-50 flex items-center justify-center bg-black/65 p-4"
          onClick={() => {
            setCapturingHotkey(false);
            setSettingsOpen(false);
          }}
        >
          <div
            onClick={(e) => e.stopPropagation()}
            className="w-full max-w-md rounded-xl border border-[#2F3746] bg-[#14161A] p-5 shadow-2xl"
          >
            <div className="mb-4 flex items-center justify-between border-b border-[#252A34] pb-3">
              <div className="flex items-center gap-2">
                <SlidersHorizontal className="h-4 w-4 text-[#38BDF8]" />
                <h3 className="text-sm font-semibold text-[#ECEFF4]">
                  KorektaBar — Ustawienia
                </h3>
              </div>
              <button
                type="button"
                onClick={() => {
                  setCapturingHotkey(false);
                  setSettingsOpen(false);
                }}
                className="text-xs text-[#94A3B8] hover:text-white"
              >
                Zamknij
              </button>
            </div>

            <div className="space-y-4 text-xs">
              <div>
                <label className="mb-1.5 block font-medium text-[#CBD5E1]">
                  Skrót globalny (kliknij pole i wciśnij kombinację)
                </label>
                <input
                  type="text"
                  readOnly
                  value={capturingHotkey ? 'Wciśnij kombinację klawiszy…' : hotkey.label}
                  onFocus={() => setCapturingHotkey(true)}
                  onBlur={() => setCapturingHotkey(false)}
                  onKeyDown={handleHotkeyCapture}
                  className="w-full cursor-pointer rounded-md border border-[#323A4A] bg-[#1B1E24] px-3 py-2 text-center font-mono text-xs font-semibold text-[#38BDF8] focus:border-[#3B82F6] focus:outline-none"
                />
              </div>

              <div className="space-y-2">
                <div className="font-medium text-[#CBD5E1]">Zakres analizy języka polskiego</div>
                <label className="flex cursor-pointer items-center gap-2.5 text-[#E2E8F0]">
                  <input
                    type="checkbox"
                    checked={analysisOpts.enableSpelling}
                    onChange={(e) =>
                      setAnalysisOpts((p) => ({ ...p, enableSpelling: e.target.checked }))
                    }
                    className="accent-[#3B82F6]"
                  />
                  Ortografia i pułapki pisowni (czerwone)
                </label>
                <label className="flex cursor-pointer items-center gap-2.5 text-[#E2E8F0]">
                  <input
                    type="checkbox"
                    checked={analysisOpts.enablePunctuation}
                    onChange={(e) =>
                      setAnalysisOpts((p) => ({ ...p, enablePunctuation: e.target.checked }))
                    }
                    className="accent-[#3B82F6]"
                  />
                  Interpunkcja i gramatyka (błękitne)
                </label>
                <label className="flex cursor-pointer items-center gap-2.5 text-[#E2E8F0]">
                  <input
                    type="checkbox"
                    checked={analysisOpts.enableStyle}
                    onChange={(e) =>
                      setAnalysisOpts((p) => ({ ...p, enableStyle: e.target.checked }))
                    }
                    className="accent-[#3B82F6]"
                  />
                  Styl, pleonazmy i powtórzenia (bursztynowe)
                </label>
              </div>

              <div className="space-y-2 border-t border-[#232833] pt-3">
                <div className="font-medium text-[#CBD5E1]">Reguły szybkiego formatera</div>
                <label className="flex cursor-pointer items-center gap-2.5 text-[#E2E8F0]">
                  <input
                    type="checkbox"
                    checked={formatterOpts.collapseSpaces}
                    onChange={(e) =>
                      setFormatterOpts((p) => ({ ...p, collapseSpaces: e.target.checked }))
                    }
                    className="accent-[#3B82F6]"
                  />
                  Redukuj nadmiarowe spacje
                </label>
                <label className="flex cursor-pointer items-center gap-2.5 text-[#E2E8F0]">
                  <input
                    type="checkbox"
                    checked={formatterOpts.fixPunctuationSpacing}
                    onChange={(e) =>
                      setFormatterOpts((p) => ({ ...p, fixPunctuationSpacing: e.target.checked }))
                    }
                    className="accent-[#3B82F6]"
                  />
                  Porządkuj odstępy przy interpunkcji
                </label>
                <label className="flex cursor-pointer items-center gap-2.5 text-[#E2E8F0]">
                  <input
                    type="checkbox"
                    checked={formatterOpts.normalizeParagraphs}
                    onChange={(e) =>
                      setFormatterOpts((p) => ({ ...p, normalizeParagraphs: e.target.checked }))
                    }
                    className="accent-[#3B82F6]"
                  />
                  Normalizuj puste linie i akapity
                </label>
                <label className="flex cursor-pointer items-center gap-2.5 text-[#E2E8F0]">
                  <input
                    type="checkbox"
                    checked={formatterOpts.capitalizeSentences}
                    onChange={(e) =>
                      setFormatterOpts((p) => ({ ...p, capitalizeSentences: e.target.checked }))
                    }
                    className="accent-[#3B82F6]"
                  />
                  Wielkie litery na początku zdań
                </label>
              </div>

              <div className="flex justify-end gap-2 pt-2">
                <button
                  type="button"
                  onClick={() => {
                    setCapturingHotkey(false);
                    setSettingsOpen(false);
                  }}
                  className="rounded-md bg-[#2563EB] px-4 py-2 text-xs font-semibold text-white hover:bg-[#3B82F6]"
                >
                  Zatwierdź
                </button>
              </div>
            </div>
          </div>
        </div>
      )}
    </section>
  );
};
