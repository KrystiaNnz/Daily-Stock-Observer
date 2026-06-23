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
    print(json.dumps({"error": "Brakuje biblioteki yfinance. Uruchom: pip install yfinance"}))
    sys.exit(1)


def search_tickers(query: str) -> list:
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
        return results
    except Exception as exc:
        return [{"error": str(exc)}]


if __name__ == "__main__":
    if len(sys.argv) != 2 or not sys.argv[1].strip():
        print(json.dumps({"error": "Użycie: search.py <query>"}))
        sys.exit(1)

    print(json.dumps(search_tickers(sys.argv[1].strip()), ensure_ascii=False))
