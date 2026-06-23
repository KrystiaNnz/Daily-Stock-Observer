# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Projekt budowany przez **Qt Creator** (CMake + MinGW 64-bit, C++17).

```
# Konfiguracja (raz)
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

# Budowanie
cmake --build build
```

Wynikowy plik: `build/DailyStockObserver.exe`. Qt Creator używa katalogu `build/Desktop_Qt_6_11_0_MinGW_64_bit-Debugowa/`.

**Wymagane moduły Qt 6:** `Widgets`, `Sql`, `Network` (linkowane w `CMakeLists.txt`).

Brak testów automatycznych i linterów.

## Python

Skrypty w `python/` są kopiowane automatycznie do katalogu exe przy budowaniu i wywoływane przez `QProcess`. Wymagają:

```
pip install yfinance
```

| Skrypt | Wywołanie | Opis |
|--------|-----------|------|
| `portfolio.py` | `python portfolio.py CDR.WA:PLN,AAPL:USD` | Kursy live z przewalutowaniem |
| `search.py` | `python search.py <query>` | Wyszukiwanie tickerów (yf.Search) |
| `analyze.py` | `python analyze.py <ticker>` | Metryki spółki + rekomendacje |
| `edgar.py` | `python edgar.py <ticker>` | Filingi SEC EDGAR |
| `peers.py` | `python peers.py <ticker>` | Rówieśnicy branżowi (używa `peers_db.json`) |
| `analysis.py` | placeholder (Faza 8) | |
| `crypto.py` | placeholder (Faza 7) | |

Wszystkie skrypty piszą JSON na stdout i błędy na stderr. Nigdy nie drukują nic innego na stdout.

## Baza danych

SQLite pod `%APPDATA%/Kryst/DailyStockObserver/daily_stock_observer.db` (`QStandardPaths::AppDataLocation`). `DatabaseManager` to Singleton — jedyna klasa która dotyka bazy. PRAGMA `foreign_keys = ON` włączone przy starcie. Schematy tabel tworzone przez `CREATE TABLE IF NOT EXISTS` — zero migracji.

## Architektura

### Nawigacja

`MainWindow` zawiera poziomy tab bar + `QStackedWidget` z czterema stronami:

| Index | Zakładka | Widget |
|-------|----------|--------|
| 0 | 📅 Planer | `QCalendarWidget` + `QListWidget` (events) |
| 1 | 🎯 Cele | `GoalsPanel` |
| 2 | 📈 Portfel | `PortfolioPanel` |
| 3 | 🗺 Mapa | `MapPanel` |

### Styl wizualny

`AppColors` (accent, leftPanel, appBg, PatternType, patternMark) i `DisplaySettings` (WindowMode, width, height) są persystowane przez `QSettings("Kryst", "DailyStockObserver")`.

`MainWindow::applyStyle()` generuje jeden duży string QSS wyliczając wszystkie kolory pochodne (hover, pressed, selection) z trzech bazowych kolorów. **Tło tabBar i lewego panelu nie jest malowane przez CSS** — maluje je `eventFilter` przez `QPainter` + `PatternGen::generate()` (kafelkowanie: Zebra, Leopard, Butterfly, Dots, Stars).

### Wzorzec async Python subprocess

Cztery fetchery (`PortfolioFetcher`, `CompanyAnalyzer`, `PeersFetcher`, `EdgarFetcher`) działają identycznie:
1. `QProcess::start("python", {scriptPath, args})`
2. Slot `onFinished` → parsuje `readAllStandardOutput()` jako JSON
3. Emituje sygnał `xyzReady(data)` lub `xyzError(message)`
4. Jeśli process już działa — `kill()` + restart (CompanyAnalyzer) lub ignoruje wywołanie (PortfolioFetcher)

`PortfolioFetcher` dodatkowo ma cache TTL 5 minut (`isCacheValid()`).

### GoalsPanel — trzy sekcje

`GoalsPanel` wyświetla cele pogrupowane w trzy niezależne sekcje, każda z własnym `QVBoxLayout` i przyciskiem toggle:
- **Dnia** — `getGoalsForDate(date)` — zawsze widoczna
- **Tygodnia** — `getGoalsForDateRange(poniedz, niedz, excludeDate)` — zwijana
- **Długoterminowe** — `getLongTermGoals(niedziela)` — zwijana; `due_date > niedziela OR NULL`

`GoalItemWidget` (QFrame) ładuje kroki lazily przy pierwszym rozwinięciu i przebudowuje listę kroków po każdej zmianie. Każdy krok ma przycisk ✕ wywołujący `DatabaseManager::deleteStep`.

### PortfolioPanel

`QSplitter` pionowy: tabela aktywów (góra) + `QTabWidget` (dół) z:
- `CompanyAnalysisPanel` — wywołuje `CompanyAnalyzer::analyze(ticker)` po kliknięciu wiersza
- `PeersPanel` — wywołuje `PeersFetcher::fetchPeers(ticker)` + rysuje `WorldHeatmapWidget`

`WorldHeatmapWidget` to czysty `QWidget` z `paintEvent` — rysuje bąbelki na uproszczonej mapie świata per kraj (bez WebEngine).

### MapPanel

Wlasny `QWidget` z kafelkami mapy przez `TileManager` + `QNetworkAccessManager`. Brak WebEngineWidgets.

- `paintEvent` rysuje kafelki CARTO/OSM, status bar i atrybucje.
- Gesty: drag = pan, kolko myszy = zoom.
- Toolbar: wyszukiwarka kraju/miasta przez Nominatim Search, przyciski `+`, `-`, reset `Europa`.
- Klik mapy odpytuje Nominatim Reverse Geocoding, po rozpoznaniu kraju otwiera `CountryInfoDialog`.
- `CountryData` trzyma lokalna baze metadanych kraju po ISO; dialog pokazuje stolice, populacje, PKB, walute, jezyki, kontynent i powierzchnie.
- Notatki w `CountryInfoDialog` sa zapisywane per ISO w `QSettings("Kryst", "DailyStockObserver")`.
