"""
historical_prices.py - pobiera historyczne swiece cenowe przez yfinance.
Wejscie: python historical_prices.py fetch <ticker> <start> <end> <interval>
Wyjscie: JSON object na stdout.
"""

import io
import json
import sys
import warnings
from datetime import datetime, timedelta

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


def _end_exclusive(end_date: str) -> str:
    try:
        dt = datetime.strptime(end_date, "%Y-%m-%d")
        return (dt + timedelta(days=1)).strftime("%Y-%m-%d")
    except ValueError:
        return end_date


def _number(value):
    if value is None:
        return None
    try:
        if value != value:
            return None
        return float(value)
    except Exception:
        return None


def fetch(ticker: str, start: str, end: str, interval: str) -> dict:
    ticker = ticker.strip().upper()
    interval = (interval or "1d").strip()
    source = "yfinance"
    result = {
        "ticker": ticker,
        "interval": interval,
        "source": source,
        "currency": "",
        "rows": [],
    }

    try:
        t = _silent(yf.Ticker, ticker)
        try:
            fast = _silent(lambda: t.fast_info)
            result["currency"] = (fast.currency or "").upper()
        except Exception:
            result["currency"] = ""

        hist = _silent(
            t.history,
            start=start,
            end=_end_exclusive(end),
            interval=interval,
            auto_adjust=False,
            actions=False,
        )
        if hist is None or hist.empty:
            return result

        for idx, row in hist.iterrows():
            if hasattr(idx, "to_pydatetime"):
                py_dt = idx.to_pydatetime()
            else:
                py_dt = idx
            if getattr(py_dt, "tzinfo", None) is not None:
                py_dt = py_dt.replace(tzinfo=None)
            trading_date = py_dt.strftime("%Y-%m-%d")
            timestamp = py_dt.strftime("%Y-%m-%dT%H:%M:%S")

            result["rows"].append({
                "timestamp": timestamp,
                "trading_date": trading_date,
                "open": _number(row.get("Open")),
                "high": _number(row.get("High")),
                "low": _number(row.get("Low")),
                "close": _number(row.get("Close")),
                "adj_close": _number(row.get("Adj Close", row.get("Close"))),
                "volume": _number(row.get("Volume")) or 0.0,
            })
    except Exception as exc:
        result["error"] = str(exc)

    return result


def main():
    if len(sys.argv) != 6 or sys.argv[1] != "fetch":
        _stdout.write(json.dumps(
            {"error": "Uzycie: historical_prices.py fetch <ticker> <start> <end> <interval>"}
        ) + "\n")
        sys.exit(1)

    _stdout.write(json.dumps(
        fetch(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5]),
        ensure_ascii=False,
    ) + "\n")


if __name__ == "__main__":
    main()
