export type DiagnosticCategory = 'spelling' | 'punctuation' | 'style';

export interface DiagnosticSpan {
  id: string;
  start: number;
  length: number;
  category: DiagnosticCategory;
  title: string;
  explanation: string;
  suggestions: string[];
  autoFixable: boolean;
}

export interface FormatterOptions {
  collapseSpaces: boolean;
  fixPunctuationSpacing: boolean;
  normalizeParagraphs: boolean;
  capitalizeSentences: boolean;
}

export interface AnalysisOptions {
  enableSpelling: boolean;
  enablePunctuation: boolean;
  enableStyle: boolean;
}

const POLISH_UPPER = /[A-ZĄĆĘŁŃÓŚŹŻ]/;
const POLISH_LETTER = /[A-Za-zĄĆĘŁŃÓŚŹŻąćęłńóśźż]/;

function matchCase(original: string, replacement: string): string {
  if (!original || !replacement) return replacement;
  if (POLISH_UPPER.test(original[0])) {
    return replacement[0].toLocaleUpperCase('pl-PL') + replacement.slice(1);
  }
  return replacement;
}

interface TrapEntry {
  replacement: string;
  title: string;
  explanation: string;
  category: DiagnosticCategory;
  autoFixable: boolean;
}

const WORD_TRAPS: Record<string, TrapEntry> = {
  wogóle: { replacement: 'w ogóle', title: 'Pisownia rozłączna', explanation: 'Wyrażenie przyimkowe „w ogóle” piszemy zawsze rozłącznie.', category: 'spelling', autoFixable: true },
  wogole: { replacement: 'w ogóle', title: 'Pisownia rozłączna i znaki polskie', explanation: 'Poprawna forma to „w ogóle” (rozłącznie i z „ó”).', category: 'spelling', autoFixable: true },
  wogule: { replacement: 'w ogóle', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „w ogóle”.', category: 'spelling', autoFixable: true },
  napewno: { replacement: 'na pewno', title: 'Pisownia rozłączna', explanation: 'Wyrażenie „na pewno” piszemy rozłącznie.', category: 'spelling', autoFixable: true },
  narazie: { replacement: 'na razie', title: 'Pisownia rozłączna', explanation: 'Wyrażenie „na razie” piszemy rozłącznie.', category: 'spelling', autoFixable: true },
  spowrotem: { replacement: 'z powrotem', title: 'Pisownia rozłączna', explanation: 'Poprawna forma to „z powrotem”.', category: 'spelling', autoFixable: true },
  zprzed: { replacement: 'sprzed', title: 'Błąd ortograficzny', explanation: 'Przyimek „sprzed” piszemy przez „s” łącznie.', category: 'spelling', autoFixable: true },
  zkąd: { replacement: 'skąd', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „skąd”.', category: 'spelling', autoFixable: true },
  wziąść: { replacement: 'wziąć', title: 'Błąd leksykalny', explanation: 'Czasownik brzmi „wziąć” (nie „wziąść”).', category: 'spelling', autoFixable: true },
  wziąśc: { replacement: 'wziąć', title: 'Błąd leksykalny', explanation: 'Poprawna forma to „wziąć”.', category: 'spelling', autoFixable: true },
  włanczać: { replacement: 'włączać', title: 'Błąd słowotwórczy', explanation: 'Poprawna forma to „włączać” (od „łączyć”).', category: 'spelling', autoFixable: true },
  włanczam: { replacement: 'włączam', title: 'Błąd słowotwórczy', explanation: 'Poprawna forma to „włączam”.', category: 'spelling', autoFixable: true },
  włancza: { replacement: 'włącza', title: 'Błąd słowotwórczy', explanation: 'Poprawna forma to „włącza”.', category: 'spelling', autoFixable: true },
  wyłanczać: { replacement: 'wyłączać', title: 'Błąd słowotwórczy', explanation: 'Poprawna forma to „wyłączać”.', category: 'spelling', autoFixable: true },
  poszłem: { replacement: 'poszedłem', title: 'Błąd fleksyjny', explanation: 'Poprawna forma czasu przeszłego rodzaju męskiego to „poszedłem”.', category: 'punctuation', autoFixable: true },
  szłem: { replacement: 'szedłem', title: 'Błąd fleksyjny', explanation: 'Poprawna forma to „szedłem”.', category: 'punctuation', autoFixable: true },
  przyszłem: { replacement: 'przyszedłem', title: 'Błąd fleksyjny', explanation: 'Poprawna forma to „przyszedłem”.', category: 'punctuation', autoFixable: true },
  wyszłem: { replacement: 'wyszedłem', title: 'Błąd fleksyjny', explanation: 'Poprawna forma to „wyszedłem”.', category: 'punctuation', autoFixable: true },
  przynajmiej: { replacement: 'przynajmniej', title: 'Literówka', explanation: 'Brakuje litery „n”: „przynajmniej”.', category: 'spelling', autoFixable: true },
  bynajmiej: { replacement: 'bynajmniej', title: 'Literówka', explanation: 'Brakuje litery „n”: „bynajmniej”.', category: 'spelling', autoFixable: true },
  pujść: { replacement: 'pójść', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „pójść” przez „ó”.', category: 'spelling', autoFixable: true },
  pójśc: { replacement: 'pójść', title: 'Brak znaku diakrytycznego', explanation: 'Poprawna forma to „pójść”.', category: 'spelling', autoFixable: true },
  kłucić: { replacement: 'kłócić', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „kłócić” przez „ó”.', category: 'spelling', autoFixable: true },
  kłucic: { replacement: 'kłócić', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „kłócić”.', category: 'spelling', autoFixable: true },
  żecz: { replacement: 'rzecz', title: 'Błąd ortograficzny', explanation: 'Słowo „rzecz” piszemy przez „rz”.', category: 'spelling', autoFixable: true },
  żeczy: { replacement: 'rzeczy', title: 'Błąd ortograficzny', explanation: 'Słowo „rzeczy” piszemy przez „rz”.', category: 'spelling', autoFixable: true },
  żeczywiscie: { replacement: 'rzeczywiście', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „rzeczywiście”.', category: 'spelling', autoFixable: true },
  rzeczywiscie: { replacement: 'rzeczywiście', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „rzeczywiście”.', category: 'spelling', autoFixable: true },
  oczywiscie: { replacement: 'oczywiście', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „oczywiście”.', category: 'spelling', autoFixable: true },
  szczegulnie: { replacement: 'szczególnie', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „szczególnie” przez „ó”.', category: 'spelling', autoFixable: true },
  szczegolnie: { replacement: 'szczególnie', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „szczególnie”.', category: 'spelling', autoFixable: true },
  ogulnie: { replacement: 'ogólnie', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „ogólnie” przez „ó”.', category: 'spelling', autoFixable: true },
  ogolnie: { replacement: 'ogólnie', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „ogólnie”.', category: 'spelling', autoFixable: true },
  póżniej: { replacement: 'później', title: 'Brak znaku diakrytycznego', explanation: 'Poprawna forma to „później”.', category: 'spelling', autoFixable: true },
  puzniej: { replacement: 'później', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „później” przez „óż”.', category: 'spelling', autoFixable: true },
  pozniej: { replacement: 'później', title: 'Brak polskich znaków', explanation: 'Poprawna forma to „później”.', category: 'spelling', autoFixable: true },
  wczesniej: { replacement: 'wcześniej', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „wcześniej”.', category: 'spelling', autoFixable: true },
  wlasnie: { replacement: 'właśnie', title: 'Brak polskich znaków', explanation: 'Poprawna forma to „właśnie”.', category: 'spelling', autoFixable: true },
  ktory: { replacement: 'który', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „który”.', category: 'spelling', autoFixable: true },
  ktora: { replacement: 'która', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „która”.', category: 'spelling', autoFixable: true },
  ktore: { replacement: 'które', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „które”.', category: 'spelling', autoFixable: true },
  ktorych: { replacement: 'których', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „których”.', category: 'spelling', autoFixable: true },
  poniewaz: { replacement: 'ponieważ', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „ponieważ”.', category: 'spelling', autoFixable: true },
  rowniez: { replacement: 'również', title: 'Brak polskich znaków', explanation: 'Poprawna forma to „również”.', category: 'spelling', autoFixable: true },
  takze: { replacement: 'także', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „także”.', category: 'spelling', autoFixable: true },
  mozna: { replacement: 'można', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „można”.', category: 'spelling', autoFixable: true },
  morzna: { replacement: 'można', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „można” przez „ż”.', category: 'spelling', autoFixable: true },
  wiecej: { replacement: 'więcej', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „więcej”.', category: 'spelling', autoFixable: true },
  bedzie: { replacement: 'będzie', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „będzie”.', category: 'spelling', autoFixable: true },
  bede: { replacement: 'będę', title: 'Brak polskich znaków', explanation: 'Poprawna forma to „będę”.', category: 'spelling', autoFixable: true },
  jestes: { replacement: 'jesteś', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „jesteś”.', category: 'spelling', autoFixable: true },
  mozemy: { replacement: 'możemy', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „możemy”.', category: 'spelling', autoFixable: true },
  musze: { replacement: 'muszę', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „muszę”.', category: 'spelling', autoFixable: true },
  dziekuje: { replacement: 'dziękuję', title: 'Brak polskich znaków', explanation: 'Poprawna forma to „dziękuję”.', category: 'spelling', autoFixable: true },
  prosze: { replacement: 'proszę', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „proszę”.', category: 'spelling', autoFixable: true },
  ksiazka: { replacement: 'książka', title: 'Brak polskich znaków', explanation: 'Poprawna forma to „książka”.', category: 'spelling', autoFixable: true },
  pieniadze: { replacement: 'pieniądze', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „pieniądze”.', category: 'spelling', autoFixable: true },
  wiadomosc: { replacement: 'wiadomość', title: 'Brak polskich znaków', explanation: 'Poprawna forma to „wiadomość”.', category: 'spelling', autoFixable: true },
  wiadomosci: { replacement: 'wiadomości', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „wiadomości”.', category: 'spelling', autoFixable: true },
  bład: { replacement: 'błąd', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „błąd”.', category: 'spelling', autoFixable: true },
  błond: { replacement: 'błąd', title: 'Błąd ortograficzny', explanation: 'Poprawna forma to „błąd” przez „ą”.', category: 'spelling', autoFixable: true },
  duzo: { replacement: 'dużo', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „dużo”.', category: 'spelling', autoFixable: true },
  zolty: { replacement: 'żółty', title: 'Brak polskich znaków', explanation: 'Poprawna forma to „żółty”.', category: 'spelling', autoFixable: true },
  zrodlo: { replacement: 'źródło', title: 'Brak polskich znaków', explanation: 'Poprawna forma to „źródło”.', category: 'spelling', autoFixable: true },
  juz: { replacement: 'już', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „już”.', category: 'spelling', autoFixable: true },
  cos: { replacement: 'coś', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „coś”.', category: 'spelling', autoFixable: true },
  ktos: { replacement: 'ktoś', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „ktoś”.', category: 'spelling', autoFixable: true },
  gdzies: { replacement: 'gdzieś', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „gdzieś”.', category: 'spelling', autoFixable: true },
  kiedys: { replacement: 'kiedyś', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „kiedyś”.', category: 'spelling', autoFixable: true },
  przegladarka: { replacement: 'przeglądarka', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „przeglądarka”.', category: 'spelling', autoFixable: true },
  zdjecie: { replacement: 'zdjęcie', title: 'Brak polskiego znaku', explanation: 'Poprawna forma to „zdjęcie”.', category: 'spelling', autoFixable: true },
  pisze: { replacement: 'jest napisane', title: 'Uwaga stylistyczna', explanation: 'W znaczeniu „w tekście widnieje” poprawniej jest użyć formy „jest napisane” lub „napisano”.', category: 'style', autoFixable: false },
};

const SUBORDINATE_CONJUNCTIONS = new Set([
  'że', 'żeby', 'aby', 'iż', 'ponieważ', 'gdyż', 'albowiem',
  'chociaż', 'choć', 'dopóki', 'zanim', 'który', 'która',
  'które', 'których', 'którym', 'którą', 'którzy', 'gdzie', 'kiedy',
]);

const COORDINATING_NO_COMMA = new Set(['i', 'oraz', 'lub', 'albo', 'czy']);

function isCompoundCommaPrefix(prevLower: string, curLower: string): boolean {
  if (curLower === 'że' || curLower === 'żeby' || curLower === 'aby') {
    return ['mimo', 'pomimo', 'chyba', 'dlatego', 'tym', 'jako'].includes(prevLower);
  }
  if (curLower === 'kiedy' && (prevLower === 'od' || prevLower === 'wtedy')) return true;
  if (curLower === 'gdzie' && prevLower === 'tam') return true;
  return false;
}

interface WordToken {
  start: number;
  length: number;
  text: string;
  lower: string;
}

export function analyzePolishText(
  text: string,
  opts: AnalysisOptions,
  ignoredWords: Set<string>,
): DiagnosticSpan[] {
  if (!text) return [];
  const spans: DiagnosticSpan[] = [];
  const tokens: WordToken[] = [];

  let i = 0;
  while (i < text.length) {
    if (POLISH_LETTER.test(text[i])) {
      const start = i;
      while (i < text.length && (POLISH_LETTER.test(text[i]) || text[i] === '-')) {
        if (text[i] === '-' && (i + 1 >= text.length || !POLISH_LETTER.test(text[i + 1]))) break;
        i++;
      }
      const word = text.slice(start, i);
      tokens.push({
        start,
        length: word.length,
        text: word,
        lower: word.toLocaleLowerCase('pl-PL'),
      });
    } else {
      i++;
    }
  }

  // Błędy wielowyrazowe i pleonazmy
  for (let t = 0; t + 1 < tokens.length; t++) {
    const a = tokens[t];
    const b = tokens[t + 1];
    const gap = text.slice(a.start + a.length, b.start);
    if (gap === ' ') {
      const pair = `${a.lower} ${b.lower}`;
      const pairLen = b.start + b.length - a.start;

      if (opts.enableSpelling && pair === 'po za') {
        spans.push({
          id: `m-${a.start}`,
          start: a.start,
          length: pairLen,
          category: 'spelling',
          title: 'Pisownia łączna',
          explanation: 'Przyimek „poza” piszemy łącznie.',
          suggestions: [matchCase(a.text, 'poza')],
          autoFixable: true,
        });
      } else if (opts.enableStyle && pair === 'dla tego') {
        spans.push({
          id: `m-${a.start}`,
          start: a.start,
          length: pairLen,
          category: 'style',
          title: 'Możliwy błąd łączności',
          explanation: 'Jeśli oznacza przyczynę/wynik, piszemy łącznie „dlatego”.',
          suggestions: [matchCase(a.text, 'dlatego')],
          autoFixable: false,
        });
      } else if (opts.enableStyle && pair === 'okres czasu') {
        spans.push({
          id: `m-${a.start}`,
          start: a.start,
          length: pairLen,
          category: 'style',
          title: 'Pleonazm („masło maślane”)',
          explanation: 'Słowo „okres” już oznacza wycinek czasu.',
          suggestions: [matchCase(a.text, 'okres')],
          autoFixable: true,
        });
      } else if (opts.enableStyle && pair === 'akwen wodny') {
        spans.push({
          id: `m-${a.start}`,
          start: a.start,
          length: pairLen,
          category: 'style',
          title: 'Pleonazm',
          explanation: '„Akwen” z definicji jest obszarem wodnym.',
          suggestions: [matchCase(a.text, 'akwen')],
          autoFixable: true,
        });
      } else if (opts.enableStyle && pair === 'fakt autentyczny') {
        spans.push({
          id: `m-${a.start}`,
          start: a.start,
          length: pairLen,
          category: 'style',
          title: 'Pleonazm',
          explanation: 'Każdy fakt jest autentyczny — wystarczy „fakt”.',
          suggestions: [matchCase(a.text, 'fakt')],
          autoFixable: true,
        });
      } else if (opts.enableStyle && pair === 'dzień dzisiejszy') {
        spans.push({
          id: `m-${a.start}`,
          start: a.start,
          length: pairLen,
          category: 'style',
          title: 'Złożoność stylistyczna',
          explanation: 'Zamiast „dzień dzisiejszy” lepiej użyć „dzisiaj” lub „obecnie”.',
          suggestions: [matchCase(a.text, 'dzisiaj')],
          autoFixable: false,
        });
      }

      if (t + 2 < tokens.length && opts.enableStyle) {
        const c = tokens[t + 2];
        if (text.slice(b.start + b.length, c.start) === ' ') {
          const tri = `${pair} ${c.lower}`;
          const triLen = c.start + c.length - a.start;
          if (tri === 'spadać w dół') {
            spans.push({
              id: `t-${a.start}`,
              start: a.start,
              length: triLen,
              category: 'style',
              title: 'Pleonazm',
              explanation: 'Spadać można wyłącznie w dół.',
              suggestions: [matchCase(a.text, 'spadać')],
              autoFixable: true,
            });
          } else if (tri === 'cofać się wstecz' || tri === 'cofnąć się wstecz') {
            spans.push({
              id: `t-${a.start}`,
              start: a.start,
              length: triLen,
              category: 'style',
              title: 'Pleonazm',
              explanation: 'Cofanie zawsze odbywa się wstecz.',
              suggestions: [matchCase(a.text, `${a.lower} się`)],
              autoFixable: true,
            });
          }
        }
      }
    }
  }

  // Pułapki wyrazowe + powtórzenia
  for (let t = 0; t < tokens.length; t++) {
    const tok = tokens[t];
    if (ignoredWords.has(tok.lower)) continue;

    if (opts.enableStyle && t > 0) {
      const prev = tokens[t - 1];
      if (prev.lower === tok.lower && text.slice(prev.start + prev.length, tok.start) === ' ') {
        spans.push({
          id: `dup-${prev.start}`,
          start: prev.start,
          length: tok.start + tok.length - prev.start,
          category: 'style',
          title: 'Powtórzenie wyrazu',
          explanation: `Zdublowane słowo „${tok.text}”.`,
          suggestions: [prev.text],
          autoFixable: true,
        });
        continue;
      }
    }

    const trap = WORD_TRAPS[tok.lower];
    if (trap) {
      const enabled =
        (trap.category === 'spelling' && opts.enableSpelling) ||
        (trap.category === 'punctuation' && opts.enablePunctuation) ||
        (trap.category === 'style' && opts.enableStyle);
      if (enabled) {
        spans.push({
          id: `w-${tok.start}`,
          start: tok.start,
          length: tok.length,
          category: trap.category,
          title: trap.title,
          explanation: trap.explanation,
          suggestions: [matchCase(tok.text, trap.replacement)],
          autoFixable: trap.autoFixable,
        });
      }
      continue;
    }

    if (opts.enableStyle && tok.length >= 5) {
      for (let back = 1; back <= 5 && t - back >= 0; back++) {
        if (tokens[t - back].lower === tok.lower) {
          spans.push({
            id: `near-${tok.start}`,
            start: tok.start,
            length: tok.length,
            category: 'style',
            title: 'Bliskie powtórzenie',
            explanation: `Wyraz „${tok.text}” pojawił się przed chwilą w tym samym fragmencie.`,
            suggestions: [],
            autoFixable: false,
          });
          break;
        }
      }
    }
  }

  // Interpunkcja
  if (opts.enablePunctuation) {
    for (let t = 1; t < tokens.length; t++) {
      const prev = tokens[t - 1];
      const cur = tokens[t];
      const gap = text.slice(prev.start + prev.length, cur.start);

      if (SUBORDINATE_CONJUNCTIONS.has(cur.lower) && !isCompoundCommaPrefix(prev.lower, cur.lower)) {
        if (/^ +$/.test(gap)) {
          spans.push({
            id: `c-${cur.start}`,
            start: cur.start,
            length: cur.length,
            category: 'punctuation',
            title: 'Brakujący przecinek',
            explanation: `Przed spójnikiem podrzędnym „${cur.lower}” stawiamy zazwyczaj przecinek.`,
            suggestions: [`, ${cur.text}`],
            autoFixable: true,
          });
        }
      }

      if (COORDINATING_NO_COMMA.has(cur.lower)) {
        const commaIdx = gap.indexOf(',');
        if (commaIdx !== -1 && !gap.includes('\n')) {
          const absPos = prev.start + prev.length + commaIdx;
          spans.push({
            id: `rc-${absPos}`,
            start: absPos,
            length: 1,
            category: 'punctuation',
            title: 'Zbędny przecinek',
            explanation: `Przed pojedynczym spójnikiem „${cur.lower}” zwykle nie stawiamy przecinka.`,
            suggestions: [''],
            autoFixable: false,
          });
        }
      }
    }

    for (let k = 0; k < text.length; k++) {
      const ch = text[k];
      if (',.!?;:'.includes(ch) && k > 0 && text[k - 1] === ' ') {
        let spStart = k - 1;
        while (spStart > 0 && text[spStart - 1] === ' ') spStart--;
        spans.push({
          id: `spb-${spStart}`,
          start: spStart,
          length: k + 1 - spStart,
          category: 'punctuation',
          title: 'Spacja przed interpunkcją',
          explanation: `Przed znakiem „${ch}” nie stawiamy spacji.`,
          suggestions: [ch],
          autoFixable: true,
        });
      }
      if (',;:'.includes(ch) && k + 1 < text.length && POLISH_LETTER.test(text[k + 1])) {
        spans.push({
          id: `spa-${k}`,
          start: k,
          length: 2,
          category: 'punctuation',
          title: 'Brak spacji po znaku',
          explanation: `Po znaku „${ch}” powinna pojawić się spacja.`,
          suggestions: [`${ch} ${text[k + 1]}`],
          autoFixable: true,
        });
      }
    }
  }

  // Wielokrotne spacje
  if (opts.enableStyle) {
    let k = 0;
    while (k < text.length) {
      if (text[k] === ' ') {
        const s = k;
        while (k < text.length && text[k] === ' ') k++;
        if (k - s >= 2 && s > 0 && text[s - 1] !== '\n') {
          spans.push({
            id: `ms-${s}`,
            start: s,
            length: k - s,
            category: 'style',
            title: 'Wielokrotna spacja',
            explanation: 'Wykryto nadmiarowe spacje obok siebie.',
            suggestions: [' '],
            autoFixable: true,
          });
        }
      } else {
        k++;
      }
    }
  }

  const order: Record<DiagnosticCategory, number> = { spelling: 0, punctuation: 1, style: 2 };
  spans.sort((a, b) => (a.start !== b.start ? a.start - b.start : order[a.category] - order[b.category]));

  const deduped: DiagnosticSpan[] = [];
  for (const sp of spans) {
    const clash = deduped.some((e) => e.start < sp.start + sp.length && sp.start < e.start + e.length);
    if (!clash) deduped.push(sp);
  }
  return deduped;
}

export function formatPolishText(input: string, opts: FormatterOptions): string {
  if (!input) return '';
  let out = input.replace(/\r\n/g, '\n').replace(/\r/g, '\n');

  if (opts.collapseSpaces) {
    out = out
      .split('\n')
      .map((line) => line.replace(/[ \t\u00A0]+/g, ' ').trim())
      .join('\n');
  }

  if (opts.normalizeParagraphs) {
    out = out.replace(/\n{3,}/g, '\n\n');
  }

  out = out.trim();

  if (opts.fixPunctuationSpacing) {
    out = out.replace(/ +([,.!?;:)\]])/g, '$1');
    out = out.replace(/([(\[]) +/g, '$1');
    out = out.replace(/([,;:])([A-Za-zĄĆĘŁŃÓŚŹŻąćęłńóśźż0-9])/g, '$1 $2');
    out = out.replace(/([.!?])([A-Za-zĄĆĘŁŃÓŚŹŻąćęłńóśźż])/g, '$1 $2');
  }

  if (opts.capitalizeSentences) {
    let needCap = true;
    const chars = Array.from(out);
    for (let i = 0; i < chars.length; i++) {
      const ch = chars[i];
      if (needCap) {
        if (POLISH_LETTER.test(ch)) {
          chars[i] = ch.toLocaleUpperCase('pl-PL');
          needCap = false;
        } else if (/[0-9]/.test(ch)) {
          needCap = false;
        } else if (!' \n"„(-—'.includes(ch)) {
          needCap = false;
        }
      } else if ('.!?'.includes(ch)) {
        const isDecimal = ch === '.' && i > 0 && i + 1 < chars.length && /[0-9]/.test(chars[i - 1]) && /[0-9]/.test(chars[i + 1]);
        if (!isDecimal) needCap = true;
      } else if (ch === '\n' && i > 0 && chars[i - 1] === '\n') {
        needCap = true;
      }
    }
    out = chars.join('');
  }

  return out;
}

export function applyAutoFixAll(text: string, spans: DiagnosticSpan[]): { text: string; fixedCount: number } {
  const fixable = spans
    .filter((s) => s.autoFixable && s.suggestions.length > 0 && s.start + s.length <= text.length)
    .sort((a, b) => b.start - a.start);

  let next = text;
  let lastStart = text.length + 1;
  let fixedCount = 0;

  for (const sp of fixable) {
    if (sp.start + sp.length <= lastStart) {
      next = next.slice(0, sp.start) + sp.suggestions[0] + next.slice(sp.start + sp.length);
      lastStart = sp.start;
      fixedCount++;
    }
  }
  return { text: next, fixedCount };
}
