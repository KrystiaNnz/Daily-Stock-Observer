"""
macro_data.py - konektory danych makro/rynkowych do standardowych tabel.

MVP:
  python macro_data.py fetch-pl-nbp <db_path> <start> <end>

Pobiera z NBP API:
  - kursy srednie tabeli A: USD, EUR, CHF, GBP -> fx_*_local
  - cene zlota -> gold_price

Wyniki zapisuje do macro_observations oraz mapowan zrodel dla Polski.
"""

import argparse
import json
import ssl
import sqlite3
import sys
import urllib.error
import urllib.request
from datetime import datetime, timedelta


NBP_BASE = "https://api.nbp.pl/api"
NBP_SOURCE_CODE = "NBP_API"
TLS_FALLBACK_USED = False
FX_INDICATORS = {
    "USD": "fx_usd_local",
    "EUR": "fx_eur_local",
    "CHF": "fx_chf_local",
    "GBP": "fx_gbp_local",
}


def _date(value):
    return datetime.strptime(value, "%Y-%m-%d").date()


def _chunks(start, end, max_days=93):
    cursor = start
    while cursor <= end:
        chunk_end = min(end, cursor + timedelta(days=max_days - 1))
        yield cursor, chunk_end
        cursor = chunk_end + timedelta(days=1)


def _verified_ssl_context():
    try:
        import certifi

        return ssl.create_default_context(cafile=certifi.where())
    except ImportError:
        return ssl.create_default_context()


def _is_cert_error(exc):
    if isinstance(exc, ssl.SSLCertVerificationError):
        return True
    reason = getattr(exc, "reason", None)
    return isinstance(reason, ssl.SSLCertVerificationError)


def _get_json(url):
    global TLS_FALLBACK_USED
    request = urllib.request.Request(
        url,
        headers={
            "Accept": "application/json",
            "User-Agent": "DailyStockObserver/0.1",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=30, context=_verified_ssl_context()) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        if exc.code == 404:
            return None
        raise
    except (urllib.error.URLError, ssl.SSLCertVerificationError) as exc:
        if not _is_cert_error(exc):
            raise
        TLS_FALLBACK_USED = True
        sys.stderr.write(
            "WARN: TLS certificate verification failed; retrying with an unverified SSL context.\n"
        )
        with urllib.request.urlopen(
            request, timeout=30, context=ssl._create_unverified_context()
        ) as response:
            return json.loads(response.read().decode("utf-8"))


def ensure_schema(conn):
    conn.executescript(
        """
        CREATE TABLE IF NOT EXISTS data_sources (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            source_code     TEXT    NOT NULL UNIQUE,
            name            TEXT    NOT NULL,
            source_type     TEXT,
            country_iso     TEXT,
            api_url         TEXT,
            website_url     TEXT,
            confidence      TEXT,
            notes           TEXT,
            updated_at      TEXT    DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS indicator_source_mappings (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            country_iso         TEXT    NOT NULL,
            indicator_code      TEXT    NOT NULL,
            source_code         TEXT    NOT NULL,
            source_indicator_id TEXT,
            source_frequency    TEXT,
            source_unit         TEXT,
            transform_hint      TEXT,
            priority            INTEGER DEFAULT 100,
            status              TEXT    DEFAULT 'planned',
            notes               TEXT,
            last_checked        TEXT,
            updated_at          TEXT    DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(country_iso, indicator_code, source_code)
        );

        CREATE TABLE IF NOT EXISTS macro_observations (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            country_iso     TEXT    NOT NULL,
            indicator_code  TEXT    NOT NULL,
            date            TEXT    NOT NULL,
            value           REAL    NOT NULL,
            unit            TEXT,
            frequency       TEXT,
            source          TEXT    NOT NULL,
            confidence      TEXT,
            updated_at      TEXT    DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(country_iso, indicator_code, date, source)
        );
        """
    )
    conn.commit()


def upsert_source(conn):
    conn.execute(
        """
        INSERT INTO data_sources (
            source_code, name, source_type, country_iso, api_url, website_url,
            confidence, notes, updated_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
        ON CONFLICT(source_code) DO UPDATE SET
            name = excluded.name,
            source_type = excluded.source_type,
            country_iso = excluded.country_iso,
            api_url = excluded.api_url,
            website_url = excluded.website_url,
            confidence = excluded.confidence,
            notes = excluded.notes,
            updated_at = CURRENT_TIMESTAMP
        """,
        (
            NBP_SOURCE_CODE,
            "Narodowy Bank Polski Web API",
            "central_bank_api",
            "PL",
            NBP_BASE,
            "https://api.nbp.pl/",
            "official",
            "Publiczne API NBP dla kursow walut i cen zlota.",
        ),
    )


def upsert_mapping(conn, indicator_code, source_indicator_id, unit, notes):
    conn.execute(
        """
        INSERT INTO indicator_source_mappings (
            country_iso, indicator_code, source_code, source_indicator_id,
            source_frequency, source_unit, transform_hint, priority, status,
            notes, last_checked, updated_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, DATE('now'), CURRENT_TIMESTAMP)
        ON CONFLICT(country_iso, indicator_code, source_code) DO UPDATE SET
            source_indicator_id = excluded.source_indicator_id,
            source_frequency = excluded.source_frequency,
            source_unit = excluded.source_unit,
            transform_hint = excluded.transform_hint,
            priority = excluded.priority,
            status = excluded.status,
            notes = excluded.notes,
            last_checked = excluded.last_checked,
            updated_at = CURRENT_TIMESTAMP
        """,
        (
            "PL",
            indicator_code,
            NBP_SOURCE_CODE,
            source_indicator_id,
            "daily",
            unit,
            "level,pct_change,zscore",
            10,
            "active",
            notes,
        ),
    )


def upsert_observation(conn, indicator_code, date, value, unit):
    conn.execute(
        """
        INSERT INTO macro_observations (
            country_iso, indicator_code, date, value, unit, frequency,
            source, confidence, updated_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
        ON CONFLICT(country_iso, indicator_code, date, source) DO UPDATE SET
            value = excluded.value,
            unit = excluded.unit,
            frequency = excluded.frequency,
            confidence = excluded.confidence,
            updated_at = CURRENT_TIMESTAMP
        """,
        ("PL", indicator_code, date, float(value), unit, "daily", NBP_SOURCE_CODE, "official"),
    )


def fetch_nbp_fx(conn, start, end):
    rows = 0
    for code, indicator in FX_INDICATORS.items():
        upsert_mapping(
            conn,
            indicator,
            f"exchangerates/rates/a/{code}",
            f"PLN_per_{code}",
            f"Sredni kurs {code}/PLN z tabeli A NBP.",
        )
        for chunk_start, chunk_end in _chunks(start, end):
            url = (
                f"{NBP_BASE}/exchangerates/rates/a/{code}/"
                f"{chunk_start.isoformat()}/{chunk_end.isoformat()}/?format=json"
            )
            payload = _get_json(url)
            if not payload:
                continue
            for item in payload.get("rates", []):
                date = item.get("effectiveDate")
                value = item.get("mid")
                if date and value is not None:
                    upsert_observation(conn, indicator, date, value, f"PLN_per_{code}")
                    rows += 1
    return rows


def fetch_nbp_gold(conn, start, end):
    rows = 0
    upsert_mapping(
        conn,
        "gold_price",
        "cenyzlota",
        "PLN_per_gram",
        "Cena 1 g zlota proby 1000 wyliczona w NBP.",
    )
    for chunk_start, chunk_end in _chunks(start, end):
        url = f"{NBP_BASE}/cenyzlota/{chunk_start.isoformat()}/{chunk_end.isoformat()}/?format=json"
        payload = _get_json(url)
        if not payload:
            continue
        for item in payload:
            date = item.get("data")
            value = item.get("cena")
            if date and value is not None:
                upsert_observation(conn, "gold_price", date, value, "PLN_per_gram")
                rows += 1
    return rows


def fetch_pl_nbp(args):
    start = _date(args.start)
    end = _date(args.end)
    if start > end:
        return {"error": "Data start musi byc <= end."}

    conn = sqlite3.connect(args.db_path)
    try:
        ensure_schema(conn)
        upsert_source(conn)
        fx_rows = fetch_nbp_fx(conn, start, end)
        gold_rows = fetch_nbp_gold(conn, start, end)
        conn.commit()
        return {
            "country_iso": "PL",
            "source": NBP_SOURCE_CODE,
            "start": start.isoformat(),
            "end": end.isoformat(),
            "fx_rows": fx_rows,
            "gold_rows": gold_rows,
            "total_rows": fx_rows + gold_rows,
            "tls_fallback_used": TLS_FALLBACK_USED,
        }
    finally:
        conn.close()


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)

    pl_nbp = sub.add_parser("fetch-pl-nbp")
    pl_nbp.add_argument("db_path")
    pl_nbp.add_argument("start")
    pl_nbp.add_argument("end")

    args = parser.parse_args()
    if args.command == "fetch-pl-nbp":
        result = fetch_pl_nbp(args)
    else:
        result = {"error": "Nieznane polecenie."}

    sys.stdout.write(json.dumps(result, ensure_ascii=False) + "\n")


if __name__ == "__main__":
    main()
