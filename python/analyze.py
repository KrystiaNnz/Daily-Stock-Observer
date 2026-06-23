"""
analyze.py — kompletna analiza spółki: yfinance (metryki) + SEC EDGAR (filingi)
Wejście:  python analyze.py <ticker>
Wyjście:  JSON na stdout
"""

import sys
import json
import urllib.request
import urllib.error

try:
    import yfinance as yf
except ImportError:
    print(json.dumps({"error": "Brakuje biblioteki yfinance. Uruchom: pip install yfinance"}))
    sys.exit(1)

USER_AGENT = "DailyStockObserver krystiannicewicz777@gmail.com"


# ── Pomocnicze ────────────────────────────────────────────────

def safe(d, key, default=None):
    val = d.get(key, default)
    return default if val in (None, "None", "N/A", float("inf"), float("-inf")) else val


def fmt_large(val):
    if val is None:
        return "N/A"
    try:
        v = float(val)
        for div, suffix in ((1e12, "T"), (1e9, "B"), (1e6, "M")):
            if abs(v) >= div:
                return f"{v/div:.2f}{suffix}"
        return f"{v:.2f}"
    except (TypeError, ValueError):
        return "N/A"


def fmt_pct(val):
    if val is None:
        return "N/A"
    try:
        return f"{float(val)*100:.1f}%"
    except (TypeError, ValueError):
        return "N/A"


def fmt_x(val, dec=2):
    if val is None:
        return "N/A"
    try:
        return f"{float(val):.{dec}f}x"
    except (TypeError, ValueError):
        return "N/A"


# ── yfinance ─────────────────────────────────────────────────

def compute_ev(info: dict):
    """EV = marketCap + totalDebt - totalCash (enterpriseValue usunięty z yfinance)."""
    mc   = safe(info, "marketCap")
    debt = safe(info, "totalDebt")
    cash = safe(info, "totalCash")
    if mc is None:
        return None
    return mc + (debt or 0) - (cash or 0)


def compute_ev_ebitda(info: dict):
    ev    = compute_ev(info)
    ebitda = safe(info, "ebitda")
    if ev is None or ebitda is None or ebitda == 0:
        return None
    return ev / ebitda


def get_yfinance_data(ticker_str: str) -> dict:
    t    = yf.Ticker(ticker_str)
    info = t.info or {}

    ev = compute_ev(info)

    return {
        "name":             safe(info, "longName") or safe(info, "shortName") or ticker_str,
        "sector":           safe(info, "sector",   "N/A"),
        "industry":         safe(info, "industry", "N/A"),
        "description":      (safe(info, "longBusinessSummary") or "")[:600],
        "currency":         safe(info, "currency", "USD"),
        "market_cap":       fmt_large(safe(info, "marketCap")),
        "enterprise_value": fmt_large(ev),
        "metrics": {
            "pe_ttm":           fmt_x(safe(info, "trailingPE")),
            "pe_forward":       fmt_x(safe(info, "forwardPE")),
            "ev_ebitda":        fmt_x(compute_ev_ebitda(info)),
            "p_book":           fmt_x(safe(info, "priceToBook")),
            "gross_margin":     fmt_pct(safe(info, "grossMargins")),
            "operating_margin": fmt_pct(safe(info, "operatingMargins")),
            "net_margin":       fmt_pct(safe(info, "profitMargins")),
            "roe":              fmt_pct(safe(info, "returnOnEquity")),
            "roa":              fmt_pct(safe(info, "returnOnAssets")),
            "debt_equity":      fmt_x(safe(info, "debtToEquity"), 1),
            "current_ratio":    fmt_x(safe(info, "currentRatio"), 1),
            "quick_ratio":      fmt_x(safe(info, "quickRatio"), 1),
            "fcf":              fmt_large(safe(info, "freeCashflow")),
            "revenue_growth":   fmt_pct(safe(info, "revenueGrowth")),
            "earnings_growth":  fmt_pct(safe(info, "earningsGrowth")),
        },
        "analyst": {
            "recommendation": safe(info, "recommendationKey", "N/A"),
            "target_price":   safe(info, "targetMeanPrice"),
            "num_analysts":   safe(info, "numberOfAnalystOpinions", 0),
        },
    }


# ── SEC EDGAR ────────────────────────────────────────────────

def find_cik(ticker: str):
    """Zwraca (cik_padded_10, name) lub (None, None)."""
    try:
        req = urllib.request.Request(
            "https://www.sec.gov/files/company_tickers.json",
            headers={"User-Agent": USER_AGENT},
        )
        with urllib.request.urlopen(req, timeout=15) as r:
            data = json.loads(r.read().decode("utf-8"))
        upper = ticker.upper().split(".")[0]
        for entry in data.values():
            if entry.get("ticker", "").upper() == upper:
                return str(entry["cik_str"]).zfill(10), entry.get("title", ticker)
        return None, None
    except Exception:
        return None, None


def get_edgar_filings(ticker: str, limit: int = 10) -> list:
    cik, _ = find_cik(ticker)
    if not cik:
        return []
    try:
        req = urllib.request.Request(
            f"https://data.sec.gov/submissions/CIK{cik}.json",
            headers={"User-Agent": USER_AGENT},
        )
        with urllib.request.urlopen(req, timeout=15) as r:
            data = json.loads(r.read().decode("utf-8"))
    except Exception:
        return []

    recent       = data.get("filings", {}).get("recent", {})
    forms        = recent.get("form",                  [])
    filing_dates = recent.get("filingDate",            [])
    accessions   = recent.get("accessionNumber",       [])
    primary_docs = recent.get("primaryDocument",       [])
    descriptions = recent.get("primaryDocDescription", [])

    target = {"10-K", "10-Q", "8-K", "4"}
    results = []
    for i, form in enumerate(forms):
        if form not in target:
            continue
        acc   = accessions[i].replace("-", "") if i < len(accessions) else ""
        doc   = primary_docs[i]  if i < len(primary_docs)  else ""
        url   = (f"https://www.sec.gov/Archives/edgar/data/{int(cik)}/{acc}/{doc}"
                 if acc and doc else "")
        results.append({
            "form":        form,
            "filingDate":  filing_dates[i] if i < len(filing_dates) else "",
            "description": descriptions[i] if i < len(descriptions) else "",
            "url":         url,
        })
        if len(results) >= limit:
            break
    return results


# ── Main ─────────────────────────────────────────────────────

if __name__ == "__main__":
    if len(sys.argv) < 2 or not sys.argv[1].strip():
        print(json.dumps({"error": "Użycie: analyze.py <ticker>"}))
        sys.exit(1)

    ticker = sys.argv[1].strip()
    try:
        yf_data = get_yfinance_data(ticker)
        filings = get_edgar_filings(ticker)
        print(json.dumps({"ticker": ticker.upper(), **yf_data, "edgar_filings": filings},
                         ensure_ascii=False))
    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.exit(1)
