# Daily Stock Observer

Daily Stock Observer is a local Windows desktop productivity and finance app built with C++ and Qt 6. It combines a daily calendar, goal tracking, portfolio tools, and an experimental world map module in one native application.

The project is a work in progress and is mainly used as a hobby, learning, and portfolio project. It is developed primarily through "vibe coding": iterative experimentation, quick feedback loops, and learning by building. The current codebase focuses on desktop UI architecture, local persistence, Python integration, and finance-oriented tooling.

## Current Status

Work in progress. The core desktop structure is implemented and several modules are functional, but the app is not packaged as a finished release yet.

Implemented:

- Main hub panel with dashboard metrics and quick navigation to the major modules
- Data profiles: private and test profiles with separate SQLite databases, profile-specific settings, startup selector, and a visible test-profile badge
- Basic XP/level/streak system in the main hub, driven by completed goal steps and calculator exercises
- Daily calendar with event CRUD backed by SQLite
- Popup dialogs constrained to the main app/screen, with scrollable content for larger windows
- Goal tracking with steps, progress, long-term goals, and editable goal cards
- Customizable color and display settings stored locally, including automatic text contrast for light/dark backgrounds
- Portfolio panel with asset tracking and live quote fetching through Python/yfinance
- Ticker autocomplete for portfolio ticker fields: adding assets, historical reports, and Alpha analysis
- Local GPW ticker fallback in `python/search.py` for common Polish stocks when searching by company name
- Instrument Master schema for a refreshable global securities database, including exchanges, instruments, listings, aliases, locations, source mappings, and refresh jobs
- GPW company importer: `python/instrument_master.py import-gpw <db_path>` loads the official GPW company list into Instrument Master tables
- Company / Micro Factors dictionary seeded from `python/company_factor_definitions.json`
- Alpha / Edge Engine MVP in the portfolio panel: computes local factor links through covariance/correlation and stores results in SQLite
- Indicator dictionary architecture for the edge engine, seeded from `python/indicator_definitions.json`
- First Polish macro-data connector: `python/macro_data.py fetch-pl-nbp` imports NBP API FX and gold data into `macro_observations`
- Modular Qt widgets for calendar, planner, goals, portfolio, and map views
- Custom Qt world map panel with tile loading, pan/zoom, country search, country borders, administrative regions, and local data source metadata
- Financial calculator panel backed by a Python helper script for nominal and real investment return calculations, with an initial bridge for loading purchase-price data from portfolio assets

Planned:

- Packaging and release build workflow
- Encryption for selected local data
- Analytics and reporting through Python/pandas
- Forecasting and deeper portfolio analysis
- More complete local data connectors, starting with official statistical APIs
- Standardized macro observations feeding the Alpha / Edge Engine

## Tech Stack

- C++17
- Qt 6 Widgets
- Qt SQL with SQLite
- Qt Network
- CMake
- Python helper scripts
- yfinance and pandas for finance/data workflows

## Architecture

The app is organized around a Qt Widgets desktop shell with separate classes for each functional area.

Key components:

- `MainWindow` - main application window and navigation
- `ProfileManager` and `ProfileSelectionDialog` - data profile selection, startup preference, profile-specific data paths, and profile-specific settings names
- `DialogUtils` - shared dialog sizing/positioning helper for keeping popups inside the main app or visible screen area
- `MainHubPanel` - dashboard-style application hub with quick module access
- `DatabaseManager` - SQLite access layer and application persistence
- `CalendarView` - calendar and daily event list
- `PlannerView` - time-slot planner view
- `GoalsPanel` and `GoalsListView` - goal tracking UI
- `PortfolioPanel` and `PortfolioFetcher` - portfolio table and live quote fetching
- `AlphaEngineFetcher` - asynchronous bridge to `python/edge_engine.py`
- `FinancialCalculatorPanel` - finance calculator UI backed by `python/financial_calculator.py`
- `MapPanel` and `TileManager` - custom map rendering and map tile loading
- `CountryData`, `CountryBorders`, `CountrySubdivisions`, and `LocalDataSources` - local map metadata loaders
- `CountryInfoDialog` and `RegionInfoDialog` - non-modal information dialogs for map selections

SQLite is used for local app data. The database is stored per data profile under the app data directory, for example `profiles/private/daily_stock_observer.db` or `profiles/test/daily_stock_observer.db`. On the first private-profile run, an existing legacy database is copied into the new private profile location if the profile database does not exist yet. Python scripts are called as helper processes for workflows that benefit from the Python data ecosystem.

## Repository Layout

```text
.
├── *.cpp / *.h              # Qt/C++ application source
├── CMakeLists.txt           # CMake project configuration
├── resources.qrc            # Qt resource bundle
├── python/                  # Python helper scripts and local JSON datasets
└── README.md
```

Build outputs, IDE state, local databases, binaries, and temporary files are intentionally ignored by `.gitignore`.

## Build

Requirements:

- Qt 6 with Widgets, SQL, and Network modules
- CMake 3.16 or newer
- A C++17 compiler supported by Qt, such as MinGW on Windows
- Python 3 for optional helper scripts

Example CMake build:

```bash
cmake -S . -B build
cmake --build build
```

On Windows, use the Qt-enabled terminal or configure CMake so it can find your Qt installation.

## Python Helpers

The `python/` folder contains helper scripts and local JSON datasets used by selected modules.

Examples:

- `portfolio.py` fetches live market quotes
- `search.py` searches tickers
- `edge_engine.py` computes ticker-factor covariance/correlation links from the local SQLite database
- `indicator_definitions.json` contains the V1 economic-factor dictionary used to seed `indicator_definitions`
- `company_factor_definitions.json` contains the V1 company/micro-factor dictionary for Instrument Master, map layers, micro charts, and future Edge Engine inputs
- `macro_data.py fetch-pl-nbp <db_path> <start> <end>` imports Polish NBP API data into the standardized macro tables
- `instrument_master.py import-gpw <db_path>` imports the official GPW company list into `exchanges`, `instruments`, `instrument_listings`, `instrument_aliases`, `instrument_source_mappings`, and `instrument_refresh_jobs`
- `financial_calculator.py` calculates investment return, stock return with dividends, and real return after inflation
- `financial_calculator.py exercise <department>` generates finance practice tasks with XP rewards scaled by difficulty; the UI shows `MATEMATYKA FINANSOWA`, `INSTRUMENTY DLUZNE`, `TECHNIKI NOTOWAN GIELDOWYCH`, `INSTRUMENTY POCHODNE`, `ANALIZA PORTFELOWA`, and `ANALIZA WSKAZNIKOWA I RACHUNKOWOSC` mixes by default and can expand into concrete task types
- The debt-instruments mix starts with loan installments, zero-coupon bonds, coupon bond pricing, YTM, Macaulay duration, modified duration, and convexity
- The exchange-trading-techniques mix starts with fixing price, theoretical fixing volume, continuous-trading execution price, and tick-size validation
- The derivatives mix starts with forward payoff, option payoff, protective put, put-call parity, and delta exercises
- The portfolio-analysis mix starts with expected return, two-asset portfolio risk, beta, CAPM, WACC, Sharpe, Treynor, and Jensen alpha exercises
- The ratio-accounting mix starts with activity ratios, DuPont profitability ratios, gross margin, Gordon model, leverage, and ROC/debt-cost exercises
- Generated numeric exercises randomly require either A/B/C/D selection or direct numeric input
- `analysis.py`, `crypto.py`, and `forecast.py` are placeholders or early-stage modules
- JSON files provide map borders, administrative regions, and local data source metadata

## Why This Project Matters

Daily Stock Observer is not just a simple to-do list. It is a desktop application used to practice:

- native UI development with Qt
- local-first application design
- SQLite data modeling
- asynchronous process integration between C++ and Python
- finance-oriented data workflows
- map rendering and geospatial UI concepts

The project is intentionally transparent about its WIP state. The goal is to show practical engineering progress, architectural thinking, and an expanding feature set rather than a polished commercial product.
