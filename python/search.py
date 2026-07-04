"""
search.py — wyszukuje tickery przez yfinance
Wejście:  python search.py <query>
Wyjście:  JSON array na stdout

Przykład: python search.py "cdprojekt"
"""

import sys
import json

try:
    import yfinance as yf
except ImportError:
    yf = None


LOCAL_TICKERS = [
    {"ticker": "KGH.WA", "name": "KGHM Polska Miedz S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["kghm", "kgh", "miedz", "copper"]},
    {"ticker": "PKN.WA", "name": "ORLEN S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["orlen", "pkn", "pkn orlen"]},
    {"ticker": "PKO.WA", "name": "PKO Bank Polski S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["pko", "pkobp", "pko bp"]},
    {"ticker": "PEO.WA", "name": "Bank Pekao S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["pekao", "peo"]},
    {"ticker": "PZU.WA", "name": "Powszechny Zaklad Ubezpieczen S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["pzu"]},
    {"ticker": "CDR.WA", "name": "CD Projekt S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["cd projekt", "cdprojekt", "cdr", "cyberpunk"]},
    {"ticker": "ALE.WA", "name": "Allegro.eu S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["allegro", "ale"]},
    {"ticker": "LPP.WA", "name": "LPP S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["lpp", "reserved"]},
    {"ticker": "DNP.WA", "name": "Dino Polska S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["dino", "dnp"]},
    {"ticker": "PGE.WA", "name": "PGE Polska Grupa Energetyczna S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["pge"]},
    {"ticker": "JSW.WA", "name": "Jastrzebska Spolka Weglowa S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["jsw", "wegiel", "coal"]},
    {"ticker": "MBK.WA", "name": "mBank S.A.", "exchange": "WSE", "type": "EQUITY",
     "aliases": ["mbank", "mbk"]},
]


def local_matches(query: str) -> list:
    q = query.strip().lower()
    if not q:
        return []

    matches = []
    for item in LOCAL_TICKERS:
        ticker = item["ticker"].lower()
        name = item["name"].lower()
        aliases = [alias.lower() for alias in item.get("aliases", [])]
        if ticker.startswith(q) or q in name or any(alias.startswith(q) or q in alias for alias in aliases):
            row = {key: item[key] for key in ("ticker", "name", "exchange", "type")}
            matches.append(row)
    return matches


def dedupe(rows: list) -> list:
    seen = set()
    result = []
    for row in rows:
        ticker = row.get("ticker", "").upper()
        if not ticker or ticker in seen:
            continue
        seen.add(ticker)
        result.append(row)
    return result


def search_tickers(query: str) -> list:
    local = local_matches(query)

    if yf is None:
        return local or [{"error": "Brakuje biblioteki yfinance. Uruchom: pip install yfinance"}]

    try:
        s = yf.Search(query, max_results=8, news_count=0)
        results = []
        for q in s.quotes:
            ticker = q.get("symbol", "")
            name   = q.get("shortname") or q.get("longname") or ticker
            if not ticker:
                continue
            results.append({
                "ticker":   ticker,
                "name":     name,
                "exchange": q.get("exchange", ""),
                "type":     q.get("quoteType", "EQUITY"),
            })
        return dedupe(local + results)[:8]
    except Exception as exc:
        return local or [{"error": str(exc)}]


if __name__ == "__main__":
    if len(sys.argv) != 2 or not sys.argv[1].strip():
        print(json.dumps({"error": "Użycie: search.py <query>"}))
        sys.exit(1)

    print(json.dumps(search_tickers(sys.argv[1].strip()), ensure_ascii=False))
