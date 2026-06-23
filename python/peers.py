"""
peers.py — zwraca spółki peers z tej samej branży/sektora
Wejście:  python peers.py <ticker>
Wyjście:  JSON na stdout
"""
import sys
import json
import os

try:
    import yfinance as yf
except ImportError:
    print(json.dumps({"error": "Brak yfinance. pip install yfinance"}))
    sys.exit(1)

DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "peers_db.json")


def load_db():
    try:
        with open(DB_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return []


def get_peers(ticker_raw: str) -> dict:
    ticker = ticker_raw.strip().upper()
    t = yf.Ticker(ticker)
    info = t.info or {}

    sector   = (info.get("sector")   or "").strip()
    industry = (info.get("industry") or "").strip()

    base = ticker.split(".")[0]

    db = load_db()
    peers_industry = []
    peers_sector   = []

    for c in db:
        if c.get("ticker", "").upper() == base:
            continue
        if industry and c.get("industry", "") == industry:
            peers_industry.append(c)
        elif sector and c.get("sector", "") == sector and c.get("industry", "") != industry:
            peers_sector.append(c)

    return {
        "ticker":          ticker,
        "sector":          sector,
        "industry":        industry,
        "peers_industry":  peers_industry,
        "peers_sector":    peers_sector,
    }


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"error": "Użycie: peers.py <ticker>"}))
        sys.exit(1)

    ticker = sys.argv[1].strip()
    try:
        result = get_peers(ticker)
        print(json.dumps(result, ensure_ascii=False))
    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.exit(1)
