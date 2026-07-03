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
| `financial_calculator.py` | `python financial_calculator.py investment <zysk> <naklad> <inflacja_pct>` lub `stock <cena_zakupu> <cena_sprzedazy> <dywidenda> <inflacja_pct>` lub `exercise <department>` | Stopa zwrotu nominalna/realna oraz generator zadan |
| `analysis.py` | placeholder (Faza 8) | |
| `crypto.py` | placeholder (Faza 7) | |

Wszystkie skrypty piszą JSON na stdout i błędy na stderr. Nigdy nie drukują nic innego na stdout.

## Profile danych

`ProfileManager` obsluguje dwa stale profile danych: `private` / Prywatny oraz `test` / Testowy. Obsluguje tez preferencje jezyka aplikacji w ustawieniach launchera: `pl` / Polski oraz `en` / English.

Przy pierwszym starcie albo po wlaczeniu opcji `Pytaj o profil przy starcie` aplikacja pokazuje `ProfileSelectionDialog` z wyborem profilu i jezyka. Sekcja `Profil danych` w `SettingsDialog` pozwala wybrac profil na kolejny start, zmienic jezyk aplikacji i wlaczyc pytanie przy starcie. Zmiana profilu z ustawien nie przeladowuje bazy w locie; wybor profilu i jezyka zapisuje sie w `QSettings("Kryst", "DailyStockObserverLauncher")` i w pelni stosuje po restarcie aplikacji. `main.cpp` ustawia domyslne `QLocale` wedlug zapisanego jezyka; pelne tlumaczenia UI wymagaja pozniejszego podpiecia `QTranslator`.

`ProfileSelectionDialog` pojawia sie przed utworzeniem `MainWindow`, wiec nie dziedziczy glownego `MainWindow::applyStyle()`. Ma wlasne `applyContrastStyle()` i nie powinien uzywac lokalnych stalych kolorow na widgetach; nowe elementy startowego dialogu stylowac przez `objectName` i ten lokalny QSS.

Profil testowy ma widoczny bezpiecznik: tytul okna `Daily Stock Observer - TEST`, napis `Daily Stock Observer [TEST]` w tab barze oraz badge `Profil testowy` w `MainHubPanel`.

## Baza danych

SQLite jest rozdzielone per profil danych: `%APPDATA%/Kryst/DailyStockObserver/profiles/<profile-id>/daily_stock_observer.db` (`QStandardPaths::AppDataLocation`). Przy pierwszym starcie profilu `private`, jesli nowa baza profilu jeszcze nie istnieje, `DatabaseManager` kopiuje stara baze z `%APPDATA%/Kryst/DailyStockObserver/daily_stock_observer.db` do profilu prywatnego. `DatabaseManager` to Singleton — jedyna klasa która dotyka bazy. PRAGMA `foreign_keys = ON` włączone przy starcie. Schematy tabel tworzone przez `CREATE TABLE IF NOT EXISTS` — zero migracji.

Bazy danych uzytkownika sa osobiste i lokalne. Nie commitowac do GitHuba plikow SQLite ani przyszlych cache'y z historycznymi cenami instrumentow (`market_price_bars`, `market_price_cache_meta`, dane typu AAPL/S&P500 itd.). Repo ma zawierac kod, schematy, migracje i male statyczne zasoby referencyjne, ale nie zbudowany lokalnie cache danych rynkowych. Historyczne ceny maja byc odtwarzalne przez fetcher i przechowywane per profil poza repo.

Raporty portfela rozwijamy w kolejnosci: najpierw tabela historycznych cen, pozniej wykres. Tabela zostaje stalym narzedziem analizy, kontroli danych i eksportu do Excela/Power BI; wykres jest dodatkowa warstwa wizualna, nie zamiennik tabeli. MVP zapisuje dzienne OHLCV w `market_price_bars`, metadane cache w `market_price_cache_meta`, a pole `interval` zostaje przygotowane pod pozniejsze dane godzinowe/minutowe. `HistoricalPriceChartWidget` rysuje wykres nad tabela i ma tryby: linia close, slupki close oraz swiece OHLC. Widget uzywa palety aplikacji, zeby utrzymac kontrast przy jasnym/ciemnym tle. `HistoricalPriceAnalysisDialog` jest duzym widokiem analitycznym otwieranym z przycisku `Rozszerz`; pokazuje ten sam dataset z filtrem dat, statystykami i kopiowaniem/eksportem CSV.

## Architektura

### Nawigacja

`MainWindow` zawiera poziomy tab bar + `QStackedWidget` z głównymi stronami aplikacji:

| Index | Zakładka | Widget |
|-------|----------|--------|
| 0 | Główny | `MainHubPanel` |
| 1 | 📅 Planer | `QCalendarWidget` + `QListWidget` (events) |
| 2 | 🎯 Cele | `GoalsPanel` |
| 3 | 📈 Portfel | `PortfolioPanel` |
| 4 | 🗺 Mapa | `MapPanel` |
| 5 | Kalkulator | `FinancialCalculatorPanel` |

`MainHubPanel` jest dashboardem startowym: pokazuje metryki dnia i portfela oraz emituje `navigateRequested(index)`, które `MainWindow` obsługuje przez `switchTab(index)`.

Main hub pokazuje też podstawowy system XP: `DatabaseManager::toggleStep(stepId, true)` przy pierwszym wykonaniu kroku dopisuje `xp_events` i zwiększa `xp_profile.total_xp`. Każdy krok może dać XP tylko raz (`xp_events.step_id UNIQUE`), a odznaczenie kroku nie odejmuje doświadczenia. Level jest wyliczany z `total_xp`, z wykładniczo rosnącym progiem `100 * 1.35^(level-1)`.

Kalkulator moze przyznawac XP przez `DatabaseManager::awardExperience(source, xp, reference)`. Generator `exercise <department>` tworzy zadania treningowe i zwraca `xp_reward`; poprawna odpowiedz w panelu kalkulatora zapisuje zdarzenie w `xp_activity_events`. UI domyslnie pokazuje dzialy-miksy `MATEMATYKA FINANSOWA` (`financial_math_mix`), `INSTRUMENTY DLUZNE` (`debt_instruments_mix`), `TECHNIKI NOTOWAN GIELDOWYCH` (`exchange_trading_techniques_mix`), `INSTRUMENTY POCHODNE` (`derivatives_mix`), `ANALIZA PORTFELOWA` (`portfolio_analysis_mix`) oraz `ANALIZA WSKAZNIKOWA I RACHUNKOWOSC` (`ratio_accounting_mix`), a checkbox "Pokaz konkretne typy" przelacza liste na pojedyncze typy zadan. Generator zwraca tez `choices`, `correct_choice` i losowe `answer_mode`; nawet dla zadan liczbowych aplikacja losowo wymaga albo A/B/C/D, albo recznego wpisania wyniku. Alias `financial_math` wskazuje na miks matematyki finansowej. Skala matematyki finansowej: proste stopy realne/nominalne `5 XP`, stopa z akcji/kapitalizacja/FV/PV `10 XP`, renty i renta wieczysta `20 XP`, NPV/PI/EAA `25 XP`, zdyskontowany okres zwrotu `30 XP`, IRR/MIRR `35 XP`. Skala instrumentow dluznych: zero-coupon price/nominal `10 XP`, zero-coupon YTM i odsetki raty kapitalowej `15 XP`, rata annuitetowa i cena obligacji kuponowej `20 XP`, duration/modified duration `25 XP`, convexity `35 XP`. Skala TNG: krok notowan `10 XP`, realizacja kupna/sprzedazy w notowaniach ciaglych `15 XP`, teoretyczny wolumen fixingu `20 XP`, kurs fixingowy `25 XP`. Skala pochodnych: forward payoff `10 XP`, payoff call/put `15 XP`, protective put i delta `20 XP`, put-call parity `25 XP`. Skala analizy portfelowej: oczekiwana stopa zwrotu `10 XP`, beta/Sharpe/Treynor `15 XP`, CAPM i Jensen alpha `20 XP`, ryzyko portfela 2 aktywow i WACC `25 XP`. Skala analizy wskaznikowej: okres obrotu zapasami/CCC/marza brutto/DTL `10 XP`, sredni okres inkasa/DuPont `15 XP`, DOL `20 XP`, model Gordona i koszty stale z DOL `25 XP`, koszt dlugu z ROC `35 XP`.

`FinancialCalculatorPanel` ma dwa tryby źródła danych: ręczne wpisywanie oraz zaciągnięcie danych z portfela. Obecny most portfelowy pobiera z `DatabaseManager::getAllAssets()` wybrane aktywo i uzupełnia kalkulator akcji średnią ceną zakupu (`avg_buy_price`) jako `C_k`; cena sprzedaży/current price, dywidendy i dalszy model danych portfelowych pozostają do zaprojektowania później.

### Dialogi

`DialogUtils::constrainToParent(QDialog*)` ogranicza wyskakujace okna do ok. 92% rozmiaru glownego okna aplikacji albo ekranu, a nastepnie centruje je w tym obszarze. Uzywac go w nowych dialogach po `setupUi()`.

Wieksze dialogi powinny miec strukture: zewnetrzny `QVBoxLayout`, `QScrollArea` z trescia jako pierwszy element oraz `QDialogButtonBox` poza scrollem na dole. Ten wzorzec jest juz zastosowany w `SettingsDialog`, `CountryInfoDialog`, `RegionInfoDialog` i `AddAssetDialog`.

### Styl wizualny

`AppColors` (accent, leftPanel, appBg, surface, border, text, autoTextContrast, PatternType, patternMark) i `DisplaySettings` (WindowMode, width, height) sa persystowane per profil przez `QSettings("Kryst", ProfileManager::profileSettingsAppName())`, np. `DailyStockObserver_private` albo `DailyStockObserver_test`. Domyslna paleta bazuje na Notarity: `#d8e1e6`, `#f5f5f6`, `#d3d7dd`, `#3a2d97`, `#adadc2`, `#000000`; dialog ustawien ma tez przycisk `Preset Notarity`.

Checkbox `Automatyczny kontrast tekstu` jest domyslnie wlaczony. Gdy jest wlaczony, `MainWindow::applyStyle()` ignoruje reczny kolor `text` i liczy kontrast osobno dla tla aplikacji, powierzchni/kart oraz lewego panelu. Stale szarosci pomocnicze w glownym QSS sa wyprowadzane z aktywnego tla, zeby tekst nie zlewal sie z jasnym albo ciemnym motywem.

Zasada dla nowych paneli i dialogow: nie wpisywac lokalnie stalych kolorow tekstu typu `#777`, `#999`, `#bbb` ani bialego tekstu bez jawnego tla. Preferowac `setObjectName(...)` i dopisanie stylu w `MainWindow::applyStyle()`, gdzie dostepne sa kolory `bgTextColor`, `surfaceTextColor`, `fieldText`, `surfaceMutedText` itd. Bazowy QSS jest ustawiany przez `qApp->setStyleSheet(appStyle)`, wiec obejmuje tez wyskakujace okna. Kontrolki formularzy (`QLineEdit`, `QTextEdit`, `QPlainTextEdit`, `QComboBox`, `QAbstractSpinBox`), tabele, taby, checkboxy/radio i lista celow maja juz globalny styl kontrastowy. `SettingsDialog` ma dodatkowo `applyContrastStyle()`, bo zawiera wiele wlasnych sekcji i podgladow.

Dla `QSpinBox` / `QAbstractSpinBox` zostawiac rezerwe na przyciski strzalek: globalny QSS ustawia `padding-right: 28px` i szerokosc subkontrolek strzalek, a pola z sufiksem typu `px` nie powinny byc zbyt waskie. W `SettingsDialog` spinboxy rozmiaru okna maja 112 px szerokosci, zeby wartosci typu `1440 px` nie nachodzily na strzalki.

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

`PortfolioAsset` ma `assetType` rozdzielony od `category`: typ instrumentu to m.in. `Akcja`, `ETF`, `Obligacja rządowa`, `Obligacja korporacyjna`, `Kryptowaluta`, `Kontrakt na surowce`, `Inne`, a `category` zostaje sektorem/tematem. `portfolio_assets.asset_type` jest migrowane przez `ALTER TABLE ... ADD COLUMN` z domyślnym `Akcja`. `PortfolioPanel` pokazuje kolumnę `Typ` i ma osobne filtry po typie oraz kategorii. `AddAssetDialog` pokazuje podpowiedzi tickerów per typ instrumentu: ETF-y i kontrakty surowcowe używają tickerów giełdowych/Yahoo Finance, krypto zwykle `BTC-USD`, a obligacje można wpisać po tickerze, ISIN albo własnym identyfikatorze. Pojedyncze obligacje są pomijane przy automatycznym `PortfolioFetcher::fetchPrices()`.

`WorldHeatmapWidget` to czysty `QWidget` z `paintEvent` — rysuje bąbelki na uproszczonej mapie świata per kraj (bez WebEngine).

### MapPanel

Wlasny `QWidget` z kafelkami mapy przez `TileManager` + `QNetworkAccessManager`. Brak WebEngineWidgets.

- `paintEvent` rysuje kafelki CARTO/OSM, status bar i atrybucje.
- Gesty: drag = pan, kolko myszy = zoom.
- Toolbar: wyszukiwarka kraju/miasta przez Nominatim Search, przyciski `+`, `-`, reset `Europa`.
- Klik mapy odpytuje Nominatim Reverse Geocoding, po rozpoznaniu kraju otwiera `CountryInfoDialog`.
- `CountryData` trzyma lokalna baze metadanych kraju po ISO; dialog pokazuje stolice, populacje, PKB, walute, jezyki, kontynent i powierzchnie.
- Notatki w `CountryInfoDialog` sa zapisywane per ISO i per profil w `QSettings("Kryst", ProfileManager::profileSettingsAppName())`.
