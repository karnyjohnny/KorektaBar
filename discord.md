# ✍️ KorektaBar — błyskawiczny korektor i formater języka polskiego dla Windows 10

Masz dość ciężkich programów i wtyczek przeglądarkowych zjadających setki megabajtów RAM tylko po to, żeby poprawić literówkę, brakujący przecinek albo rozjechane spacje przed wysłaniem wiadomości?

**KorektaBar** to ultra-lekka, natywna aplikacja w **C++20 (Win32 API + Direct2D / DirectWrite)** stworzona z myślą o **zerowej latencji** i słabszych komputerach (testowana pod budżet **Intel Celeron N3350**).

> ⚡ **Start w ułamku sekundy (`< 15 ms`) · Zużycie ~18–35 MB RAM · Zero Electrona, zero Javy, zero reklam**

---

## ✨ Co potrafi KorektaBar?

- 🎯 **Wywołanie z dowolnego miejsca (`Ctrl + Space`)** — aplikacja siedzi cicho w zasobniku systemowym (`~6 MB RAM` w tle) i wyskakuje w stylu kompaktowej klawiatury ekranowej dokładnie **10 px nad paskiem zadań**.
- 🌑 **Nowoczesny Dark Mode** — ciemny grafit zaprojektowany pod długą pracę z tekstem, z czytelnym kodowaniem błędów:
  - 🔴 **Czerwona fala** — ortografia, literówki, pisownia łączna i rozłączna (`wogóle → w ogóle`, `napewno → na pewno`, `wziąść → wziąć`) oraz brakujące polskie znaki.
  - 🔵 **Błękitna fala** — interpunkcja i gramatyka (brakujące przecinki przed *że, żeby, aby, ponieważ, który*, zbędne przecinki przed *i / lub*, błędy fleksyjne).
  - 🟡 **Bursztynowa fala** — styl, powtórzenia sąsiednie/bliskie oraz pleonazmy (*okres czasu*, *akwen wodny*, *fakt autentyczny*).
- 🖱️ **Menu pod PPM** — kliknij prawym przyciskiem myszy na podkreślony wyraz, aby zobaczyć krótkie wyjaśnienie i podmienić błąd **jednym kliknięciem**.
- 🚀 **„Auto-popraw wszystko”** — bezpiecznie naprawia pewne błędy w całym tekście naraz (jako jeden krok `Ctrl + Z`).
- 🧹 **Szybki formater tekstu** — jednym kliknięciem usuwa wielokrotne spacje, spacje przed znakami interpunkcyjnymi, brakujące spacje po przecinkach, zbędne puste linie i poprawia wielkie litery na początku zdań.
- ⚙️ **Pełna konfiguracja** — zmiana globalnego skrótu klawiszowego, przełączniki reguł oraz pełna obsługa `Undo / Redo`, schowka i skalowania DPI.

---

## 🧠 Zaprojektowane pod wydajność

- **Natywny słownik Windows (`ISpellChecker pl-PL`) + wbudowana baza pułapek PL** — działa szybko i niezawodnie bez ładowania ciężkich silników JVM.
- **Dwuwątkowa architektura z *debounce* (300 ms)** — analizator gramatyki i stylu nie blokuje pisania nawet na 2-rdzeniowych procesorach.
- **Pojedynczy plik `.exe`** — bez instalatora i zewnętrznych bibliotek DLL.

---

## 📥 Pobieranie i skróty

- **`Ctrl + Space`** — Pokaż / ukryj pasek KorektaBar *(konfigurowalne w Opcjach)*
- **`Esc`** — Ukryj z powrotem do zasobnika
- **`PPM` na podkreśleniu** — Wyjaśnienie błędu i szybka zamiana
- **`Ctrl + Z` / `Ctrl + Y`** — Cofnij / Ponów

🔗 **Pobierz gotowe wydanie z GitHub Releases lub skompiluj samodzielnie w MSYS2!**
*(Tutaj wrzuć link do repozytorium / Releases)*
