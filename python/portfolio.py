"""
portfolio.py — pobiera kursy akcji przez yfinance z opcjonalnym przewalutowaniem
Wejście:  python portfolio.py <ticker1:waluta,ticker2:waluta,...>
Wyjście:  JSON array na stdout

Przykład: python portfolio.py CDR.WA:PLN,TTWO:PLN,AAPL:USD
Instalacja: pip install yfinance
"""

import sys
import io
import json
import warnings
from datetime import datetime

warnings.filterwarnings("ignore")
_stdout = sys.stdout


def _silent(fn, *args, **kwargs):
    buf = io.StringIO()
    old_out, old_err = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = buf
    try:
        return fn(*args, **kwargs)
    finally:
        sys.stdout = old_out
        sys.stderr = old_err


try:
    import yfinance as yf
except ImportError:
    _stdout.write(json.dumps(
        {"error": "Brakuje biblioteki yfinance. Uruchom: pip install yfinance"}
    ) + "\n")
    sys.exit(1)

# Cache kursów forex w ramach jednego wywołania skryptu
_forex_cache: dict[str, float] = {}


def get_exchange_rate(from_ccy: str, to_ccy: str) -> float:
    """Zwraca kurs: 1 from_ccy = X to_ccy. Cache w ramach wywołania."""
    if from_ccy == to_ccy:
        return 1.0
    key = f"{from_ccy}{to_ccy}"
    if key in _forex_cache:
        return _forex_cache[key]
    try:
        pair = f"{from_ccy}{to_ccy}=X"
        t    = _silent(yf.Ticker, pair)
        fast = _silent(lambda: t.fast_info)
        rate = fast.last_price
        if rate and rate > 0:
            _forex_cache[key] = rate
            return rate
    except Exception:
        pass
    return 1.0


def fetch_quote(ticker_str: str, buy_currency: str) -> dict:
    base = {
        "ticker":           ticker_str,
        "name":             ticker_str,
        "price":            None,
        "price_converted":  None,   # cena live w walucie zakupu
        "change_pct":       0.0,
        "currency":         buy_currency,
        "live_currency":    buy_currency,
        "exchange_rate":    1.0,
        "timestamp":        datetime.now().isoformat(timespec="seconds"),
        "valid":            False,
    }
    try:
        t = _silent(yf.Ticker, ticker_str)

        fast       = _silent(lambda: t.fast_info)
        price      = fast.last_price
        prev_close = fast.previous_close
        live_ccy   = (fast.currency or buy_currency).upper()

        name = ticker_str
        if price is not None:
            info = _silent(lambda: t.info)
            name = info.get("shortName") or info.get("longName") or ticker_str

        change_pct = 0.0
        if price and prev_close and prev_close > 0:
            change_pct = (price - prev_close) / prev_close * 100.0

        # Przewalutowanie jeśli waluta live != waluta zakupu
        rate            = get_exchange_rate(live_ccy, buy_currency.upper())
        price_converted = round(price * rate, 4) if price is not None else None

        base.update({
            "name":            name,
            "price":           round(price, 4) if price is not None else None,
            "price_converted": price_converted,
            "change_pct":      round(change_pct, 2),
            "currency":        buy_currency.upper(),
            "live_currency":   live_ccy,
            "exchange_rate":   round(rate, 6),
            "valid":           price is not None,
        })
    except Exception as exc:
        base["error"] = str(exc)

    return base


def main():
    if len(sys.argv) != 2:
        _stdout.write(json.dumps(
            {"error": "Użycie: portfolio.py <ticker1:waluta,...>"}
        ) + "\n")
        sys.exit(1)

    results = []
    for entry in sys.argv[1].split(","):
        entry = entry.strip()
        if not entry:
            continue
        if ":" in entry:
            ticker, buy_ccy = entry.split(":", 1)
        else:
            ticker, buy_ccy = entry, "PLN"
        results.append(fetch_quote(ticker.upper(), buy_ccy.upper()))

    _stdout.write(json.dumps(results, ensure_ascii=False) + "\n")


if __name__ == "__main__":
    main()
