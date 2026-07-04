"""
edge_engine.py - MVP silnika Alpha / Edge Engine.

Wejscie:
  python edge_engine.py compute-factor-links <db_path> <ticker> [--top N]

Skrypt czyta lokalna baze SQLite, liczy relacje ticker-czynnik na podstawie
historycznych cen oraz ustandaryzowanych obserwacji makro, zapisuje wyniki do
asset_factor_covariances i zwraca JSON na stdout.
"""

import argparse
import json
import math
import sqlite3
import sys
from datetime import datetime, timedelta


def _parse_date(value):
    if not value:
        return None
    try:
        return datetime.strptime(value[:10], "%Y-%m-%d").date()
    except ValueError:
        return None


def _mean(values):
    return sum(values) / len(values) if values else 0.0


def _covariance(xs, ys):
    n = len(xs)
    if n < 2:
        return None
    mx = _mean(xs)
    my = _mean(ys)
    return sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / (n - 1)


def _correlation(xs, ys):
    cov = _covariance(xs, ys)
    if cov is None:
        return None, None
    sx = math.sqrt(_covariance(xs, xs) or 0.0)
    sy = math.sqrt(_covariance(ys, ys) or 0.0)
    if sx <= 0.0 or sy <= 0.0:
        return cov, None
    return cov, cov / (sx * sy)


def _returns_from_levels(levels):
    rows = sorted((d, v) for d, v in levels.items() if v is not None and v > 0)
    out = {}
    prev_date = None
    prev_value = None
    for date, value in rows:
        if prev_value and prev_value > 0:
            out[date] = (value / prev_value) - 1.0
        prev_date = date
        prev_value = value
    return out


def _changes_from_levels(levels):
    rows = sorted((d, v) for d, v in levels.items() if v is not None)
    out = {}
    prev_value = None
    for date, value in rows:
        if prev_value is not None:
            out[date] = value - prev_value
        prev_value = value
    return out


def _load_market_levels(conn, ticker, window_start=None):
    sql = """
        SELECT trading_date, COALESCE(adj_close, close), close
        FROM market_price_bars
        WHERE ticker = ? AND interval = '1d' AND source = 'yfinance'
        ORDER BY trading_date ASC
    """
    levels = {}
    for trading_date, adj_close, close in conn.execute(sql, (ticker.upper(),)):
        date = _parse_date(trading_date)
        if not date or (window_start and date < window_start):
            continue
        value = adj_close if adj_close not in (None, 0) else close
        if value is not None:
            levels[date] = float(value)
    return levels


def _load_market_factor_codes(conn, target_ticker):
    sql = """
        SELECT DISTINCT ticker
        FROM market_price_bars
        WHERE interval = '1d' AND source = 'yfinance' AND ticker != ?
        ORDER BY ticker ASC
    """
    return [row[0] for row in conn.execute(sql, (target_ticker.upper(),))]


def _load_macro_factor_codes(conn):
    sql = """
        SELECT DISTINCT country_iso, indicator_code, source
        FROM macro_observations
        ORDER BY country_iso, indicator_code, source
    """
    return [f"{iso}:{code}:{source}" for iso, code, source in conn.execute(sql)]


def _load_macro_changes(conn, factor_code, window_start=None):
    try:
        country_iso, indicator_code, source = factor_code.split(":", 2)
    except ValueError:
        return {}

    sql = """
        SELECT date, value
        FROM macro_observations
        WHERE country_iso = ? AND indicator_code = ? AND source = ?
        ORDER BY date ASC
    """
    levels = {}
    for date_raw, value in conn.execute(sql, (country_iso, indicator_code, source)):
        date = _parse_date(date_raw)
        if not date or (window_start and date < window_start):
            continue
        if value is not None:
            levels[date] = float(value)
    return _changes_from_levels(levels)


def _align_series(target_returns, factor_returns, lag_periods):
    target_dates = sorted(target_returns)
    factor_dates = sorted(factor_returns)
    if not target_dates or not factor_dates:
        return [], []

    if lag_periods <= 0:
        common = sorted(set(target_returns).intersection(factor_returns))
        return [target_returns[d] for d in common], [factor_returns[d] for d in common]

    shifted = {}
    for idx, date in enumerate(factor_dates):
        target_idx = idx + lag_periods
        if target_idx < len(factor_dates):
            shifted[factor_dates[target_idx]] = factor_returns[date]
    common = sorted(set(target_returns).intersection(shifted))
    return [target_returns[d] for d in common], [shifted[d] for d in common]


def _stability_score(xs, ys):
    if len(xs) < 60:
        return 0.0
    mid = len(xs) // 2
    _, c1 = _correlation(xs[:mid], ys[:mid])
    _, c2 = _correlation(xs[mid:], ys[mid:])
    if c1 is None or c2 is None:
        return 0.0
    diff = abs(c1 - c2)
    sign_bonus = 1.0 if c1 * c2 >= 0 else 0.5
    return max(0.0, min(1.0, (1.0 - min(diff, 1.0)) * sign_bonus))


def _save_results(conn, ticker, rows):
    now = datetime.now().strftime("%Y-%m-%dT%H:%M:%S")
    sql = """
        INSERT INTO asset_factor_covariances (
            ticker, factor_code, factor_type, window_days, lag_periods,
            covariance, correlation, p_value, observations, stability_score, computed_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(ticker, factor_code, factor_type, window_days, lag_periods)
        DO UPDATE SET
            covariance = excluded.covariance,
            correlation = excluded.correlation,
            p_value = excluded.p_value,
            observations = excluded.observations,
            stability_score = excluded.stability_score,
            computed_at = excluded.computed_at
    """
    for row in rows:
        conn.execute(sql, (
            ticker.upper(),
            row["factor_code"],
            row["factor_type"],
            row["window_days"],
            row["lag_periods"],
            row["covariance"],
            row["correlation"],
            None,
            row["observations"],
            row["stability_score"],
            now,
        ))
    conn.commit()


def compute_factor_links(args):
    ticker = args.ticker.upper()
    window_start = None
    if args.window_days > 0:
        window_start = datetime.now().date() - timedelta(days=args.window_days)

    conn = sqlite3.connect(args.db_path)
    try:
        target_levels = _load_market_levels(conn, ticker, window_start)
        target_returns = _returns_from_levels(target_levels)
        if len(target_returns) < args.min_observations:
            return {
                "ticker": ticker,
                "rows": [],
                "error": f"Za malo danych cenowych dla {ticker}: {len(target_returns)} obserwacji zwrotu.",
            }

        results = []

        for factor in _load_market_factor_codes(conn, ticker):
            factor_returns = _returns_from_levels(_load_market_levels(conn, factor, window_start))
            xs, ys = _align_series(target_returns, factor_returns, args.lag_periods)
            if len(xs) < args.min_observations:
                continue
            cov, corr = _correlation(xs, ys)
            if corr is None:
                continue
            results.append({
                "factor_code": factor,
                "factor_type": "market_price",
                "window_days": args.window_days,
                "lag_periods": args.lag_periods,
                "covariance": cov,
                "correlation": corr,
                "observations": len(xs),
                "stability_score": _stability_score(xs, ys),
            })

        for factor in _load_macro_factor_codes(conn):
            factor_changes = _load_macro_changes(conn, factor, window_start)
            xs, ys = _align_series(target_returns, factor_changes, args.lag_periods)
            if len(xs) < max(8, min(args.min_observations, 24)):
                continue
            cov, corr = _correlation(xs, ys)
            if corr is None:
                continue
            results.append({
                "factor_code": factor,
                "factor_type": "macro",
                "window_days": args.window_days,
                "lag_periods": args.lag_periods,
                "covariance": cov,
                "correlation": corr,
                "observations": len(xs),
                "stability_score": _stability_score(xs, ys),
            })

        results.sort(key=lambda row: abs(row["correlation"]), reverse=True)
        saved = results[: args.save_limit]
        _save_results(conn, ticker, saved)

        return {
            "ticker": ticker,
            "window_days": args.window_days,
            "lag_periods": args.lag_periods,
            "rows": saved[: args.top],
            "saved_rows": len(saved),
            "candidate_rows": len(results),
        }
    finally:
        conn.close()


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)

    factor = sub.add_parser("compute-factor-links")
    factor.add_argument("db_path")
    factor.add_argument("ticker")
    factor.add_argument("--top", type=int, default=25)
    factor.add_argument("--save-limit", type=int, default=100)
    factor.add_argument("--window-days", type=int, default=756)
    factor.add_argument("--lag-periods", type=int, default=0)
    factor.add_argument("--min-observations", type=int, default=30)

    args = parser.parse_args()
    if args.command == "compute-factor-links":
        result = compute_factor_links(args)
    else:
        result = {"error": "Nieznane polecenie"}

    sys.stdout.write(json.dumps(result, ensure_ascii=False) + "\n")


if __name__ == "__main__":
    main()
