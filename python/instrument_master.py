"""
instrument_master.py - importery instrumentow do lokalnego Instrument Master.

MVP:
  python instrument_master.py import-gpw <db_path>

Importer GPW pobiera publiczna liste spolek z gpw.pl/spolki i zapisuje:
  - exchanges
  - instruments
  - instrument_listings
  - instrument_aliases
  - instrument_source_mappings
  - instrument_refresh_jobs

Skrypt uzywa tylko standardowej biblioteki Pythona.
"""

import argparse
import html
import http.client
import json
import re
import sqlite3
import ssl
import sys
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime
from html.parser import HTMLParser


GPW_PAGE_URL = "https://www.gpw.pl/spolki"
GPW_AJAX_URL = "https://www.gpw.pl/ajaxindex.php"
GPW_SOURCE_CODE = "GPW_COMPANY_SEARCH"


class GpwFormParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.in_search_form = False
        self.form_depth = 0
        self.fields = []
        self.count_all = None
        self._capture_count = False

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == "form" and attrs.get("id") == "search-form":
            self.in_search_form = True
            self.form_depth = 1
            return

        if self.in_search_form:
            if tag == "form":
                self.form_depth += 1
            if tag == "input":
                name = attrs.get("name")
                if not name:
                    return
                input_type = attrs.get("type", "text").lower()
                checked = "checked" in attrs
                if input_type in ("hidden", "text"):
                    self.fields.append((name, attrs.get("value", "")))
                elif input_type == "checkbox" and checked:
                    self.fields.append((name, attrs.get("value", "on")))

        if tag == "span" and attrs.get("id") == "count-all":
            self._capture_count = True

    def handle_endtag(self, tag):
        if self.in_search_form and tag == "form":
            self.form_depth -= 1
            if self.form_depth <= 0:
                self.in_search_form = False

    def handle_data(self, data):
        if self._capture_count:
            try:
                self.count_all = int(data.strip())
            except ValueError:
                self.count_all = None
            self._capture_count = False


def _ssl_context():
    try:
        import certifi

        return ssl.create_default_context(cafile=certifi.where())
    except ImportError:
        return ssl.create_default_context()


def _request(url, data=None):
    headers = {
        "User-Agent": "DailyStockObserver/0.1",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
        "Referer": GPW_PAGE_URL,
    }
    encoded = None
    if data is not None:
        encoded = urllib.parse.urlencode(data).encode("utf-8")
        headers["Content-Type"] = "application/x-www-form-urlencoded"

    req = urllib.request.Request(url, data=encoded, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=30, context=_ssl_context()) as response:
            try:
                payload = response.read()
            except http.client.IncompleteRead as exc:
                payload = exc.partial
            return payload.decode("utf-8", errors="replace")
    except (urllib.error.URLError, ssl.SSLCertVerificationError) as exc:
        reason = getattr(exc, "reason", exc)
        if not isinstance(reason, ssl.SSLCertVerificationError):
            raise
        sys.stderr.write("WARN: TLS certificate verification failed; retrying unverified.\n")
        with urllib.request.urlopen(
            req, timeout=30, context=ssl._create_unverified_context()
        ) as response:
            try:
                payload = response.read()
            except http.client.IncompleteRead as exc:
                payload = exc.partial
            return payload.decode("utf-8", errors="replace")


def _normalize(value):
    return re.sub(r"\s+", " ", html.unescape(value or "")).strip()


def _alias_norm(value):
    return _normalize(value).lower()


def parse_gpw_rows(fragment):
    pattern = re.compile(
        r'<a href="spolka\?isin=([^"]+)">\s*'
        r'<strong class="name">\s*(.*?)\s*'
        r'<span class="grey">\(([^)]+)\)</span>\s*</strong>\s*</a>\s*'
        r'<small class="grey">(.*?)</small>',
        re.S,
    )
    rows = []
    for match in pattern.finditer(fragment):
        isin = _normalize(match.group(1))
        name = _normalize(re.sub(r"<[^>]+>", " ", match.group(2)))
        ticker = _normalize(match.group(3)).upper()
        meta = _normalize(re.sub(r"<[^>]+>", " ", match.group(4)))
        parts = [part.strip(" |") for part in meta.split("|")]
        market_segment = parts[0] if parts else "Glowny Rynek"
        sector = parts[-1] if len(parts) >= 2 else ""
        indexes = parts[1] if len(parts) >= 3 else ""
        if isin and ticker and name:
            rows.append({
                "isin": isin,
                "ticker": ticker,
                "provider_symbol": ticker + ".WA",
                "name": name,
                "sector": sector,
                "market_segment": market_segment,
                "indexes": indexes,
            })
    return rows


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

        CREATE TABLE IF NOT EXISTS exchanges (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            exchange_code       TEXT    NOT NULL UNIQUE,
            name                TEXT    NOT NULL,
            country_iso         TEXT,
            mic_code            TEXT,
            yahoo_suffix        TEXT,
            timezone            TEXT,
            default_currency    TEXT,
            website_url         TEXT,
            data_source         TEXT,
            active              INTEGER DEFAULT 1,
            updated_at          TEXT    DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS instruments (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            canonical_name      TEXT    NOT NULL,
            legal_name          TEXT,
            instrument_type     TEXT    DEFAULT 'equity',
            country_iso         TEXT,
            sector              TEXT,
            industry            TEXT,
            website_url         TEXT,
            active              INTEGER DEFAULT 1,
            created_at          TEXT    DEFAULT CURRENT_TIMESTAMP,
            updated_at          TEXT    DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS instrument_listings (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            instrument_id       INTEGER NOT NULL,
            exchange_id         INTEGER,
            ticker              TEXT    NOT NULL,
            provider_symbol     TEXT,
            currency            TEXT,
            isin                TEXT,
            figi                TEXT,
            lei                 TEXT,
            listing_status      TEXT    DEFAULT 'active',
            first_seen          TEXT,
            last_seen           TEXT,
            updated_at          TEXT    DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(ticker, exchange_id, provider_symbol)
        );

        CREATE TABLE IF NOT EXISTS instrument_aliases (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            instrument_id       INTEGER,
            listing_id          INTEGER,
            alias               TEXT    NOT NULL,
            alias_norm          TEXT    NOT NULL,
            alias_type          TEXT,
            priority            INTEGER DEFAULT 100,
            source              TEXT,
            updated_at          TEXT    DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(alias_norm, listing_id)
        );

        CREATE TABLE IF NOT EXISTS instrument_source_mappings (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            listing_id          INTEGER,
            source_code         TEXT    NOT NULL,
            source_symbol       TEXT,
            source_url          TEXT,
            refresh_frequency   TEXT,
            status              TEXT    DEFAULT 'planned',
            last_checked        TEXT,
            updated_at          TEXT    DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(listing_id, source_code, source_symbol)
        );

        CREATE TABLE IF NOT EXISTS instrument_refresh_jobs (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            source_code         TEXT    NOT NULL,
            exchange_id         INTEGER,
            started_at          TEXT    DEFAULT CURRENT_TIMESTAMP,
            finished_at         TEXT,
            status              TEXT    DEFAULT 'running',
            rows_fetched        INTEGER DEFAULT 0,
            rows_inserted       INTEGER DEFAULT 0,
            rows_updated        INTEGER DEFAULT 0,
            error_message       TEXT
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
            GPW_SOURCE_CODE,
            "GPW company search",
            "exchange_company_list",
            "PL",
            GPW_AJAX_URL,
            GPW_PAGE_URL,
            "official_page",
            "Publiczna lista spolek z gpw.pl/spolki.",
        ),
    )


def upsert_exchange(conn):
    conn.execute(
        """
        INSERT INTO exchanges (
            exchange_code, name, country_iso, mic_code, yahoo_suffix, timezone,
            default_currency, website_url, data_source, active, updated_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 1, CURRENT_TIMESTAMP)
        ON CONFLICT(exchange_code) DO UPDATE SET
            name = excluded.name,
            country_iso = excluded.country_iso,
            mic_code = excluded.mic_code,
            yahoo_suffix = excluded.yahoo_suffix,
            timezone = excluded.timezone,
            default_currency = excluded.default_currency,
            website_url = excluded.website_url,
            data_source = excluded.data_source,
            active = 1,
            updated_at = CURRENT_TIMESTAMP
        """,
        (
            "GPW_MAIN",
            "Gielda Papierow Wartosciowych w Warszawie - Glowny Rynek",
            "PL",
            "XWAR",
            ".WA",
            "Europe/Warsaw",
            "PLN",
            GPW_PAGE_URL,
            GPW_SOURCE_CODE,
        ),
    )
    return conn.execute("SELECT id FROM exchanges WHERE exchange_code = ?", ("GPW_MAIN",)).fetchone()[0]


def start_job(conn, exchange_id):
    cur = conn.execute(
        """
        INSERT INTO instrument_refresh_jobs (source_code, exchange_id, status, started_at)
        VALUES (?, ?, 'running', CURRENT_TIMESTAMP)
        """,
        (GPW_SOURCE_CODE, exchange_id),
    )
    return cur.lastrowid


def finish_job(conn, job_id, status, fetched, inserted, updated, error=None):
    conn.execute(
        """
        UPDATE instrument_refresh_jobs
        SET finished_at = CURRENT_TIMESTAMP,
            status = ?,
            rows_fetched = ?,
            rows_inserted = ?,
            rows_updated = ?,
            error_message = ?
        WHERE id = ?
        """,
        (status, fetched, inserted, updated, error, job_id),
    )


def upsert_company(conn, exchange_id, row):
    existing = conn.execute(
        "SELECT id, instrument_id FROM instrument_listings WHERE isin = ? OR provider_symbol = ?",
        (row["isin"], row["provider_symbol"]),
    ).fetchone()
    was_insert = existing is None

    if existing:
        listing_id, instrument_id = existing
        conn.execute(
            """
            UPDATE instruments
            SET canonical_name = ?, legal_name = ?, country_iso = COALESCE(country_iso, 'PL'),
                sector = ?, active = 1, updated_at = CURRENT_TIMESTAMP
            WHERE id = ?
            """,
            (row["name"], row["name"], row["sector"], instrument_id),
        )
    else:
        cur = conn.execute(
            """
            INSERT INTO instruments (
                canonical_name, legal_name, instrument_type, country_iso, sector,
                active, created_at, updated_at
            ) VALUES (?, ?, 'equity', 'PL', ?, 1, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
            """,
            (row["name"], row["name"], row["sector"]),
        )
        instrument_id = cur.lastrowid
        listing_id = None

    conn.execute(
        """
        INSERT INTO instrument_listings (
            instrument_id, exchange_id, ticker, provider_symbol, currency, isin,
            listing_status, first_seen, last_seen, updated_at
        ) VALUES (?, ?, ?, ?, 'PLN', ?, 'active', DATE('now'), DATE('now'), CURRENT_TIMESTAMP)
        ON CONFLICT(ticker, exchange_id, provider_symbol) DO UPDATE SET
            instrument_id = excluded.instrument_id,
            currency = excluded.currency,
            isin = excluded.isin,
            listing_status = excluded.listing_status,
            last_seen = excluded.last_seen,
            updated_at = CURRENT_TIMESTAMP
        """,
        (instrument_id, exchange_id, row["ticker"], row["provider_symbol"], row["isin"]),
    )
    if listing_id is None:
        listing_id = conn.execute(
            "SELECT id FROM instrument_listings WHERE ticker = ? AND exchange_id = ? AND provider_symbol = ?",
            (row["ticker"], exchange_id, row["provider_symbol"]),
        ).fetchone()[0]

    aliases = [
        (row["ticker"], "ticker", 5),
        (row["provider_symbol"], "provider_symbol", 5),
        (row["name"], "company_name", 20),
    ]
    short_name = re.sub(r"\b(SPOLKA|SPÓŁKA|AKCYJNA|S\.A\.|SA)\b", " ", row["name"], flags=re.I)
    short_name = _normalize(short_name)
    if short_name and short_name != row["name"]:
        aliases.append((short_name, "clean_name", 30))

    for alias, alias_type, priority in aliases:
        conn.execute(
            """
            INSERT INTO instrument_aliases (
                instrument_id, listing_id, alias, alias_norm, alias_type, priority, source, updated_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
            ON CONFLICT(alias_norm, listing_id) DO UPDATE SET
                alias = excluded.alias,
                alias_type = excluded.alias_type,
                priority = excluded.priority,
                source = excluded.source,
                updated_at = CURRENT_TIMESTAMP
            """,
            (instrument_id, listing_id, alias, _alias_norm(alias), alias_type, priority, GPW_SOURCE_CODE),
        )

    conn.execute(
        """
        INSERT INTO instrument_source_mappings (
            listing_id, source_code, source_symbol, source_url,
            refresh_frequency, status, last_checked, updated_at
        ) VALUES (?, ?, ?, ?, 'daily', 'active', DATE('now'), CURRENT_TIMESTAMP)
        ON CONFLICT(listing_id, source_code, source_symbol) DO UPDATE SET
            source_url = excluded.source_url,
            refresh_frequency = excluded.refresh_frequency,
            status = excluded.status,
            last_checked = excluded.last_checked,
            updated_at = CURRENT_TIMESTAMP
        """,
        (listing_id, GPW_SOURCE_CODE, row["isin"], f"https://www.gpw.pl/spolka?isin={row['isin']}"),
    )

    return was_insert


def fetch_gpw_companies():
    html_page = _request(GPW_PAGE_URL)
    parser = GpwFormParser()
    parser.feed(html_page)
    base_fields = parser.fields
    expected_count = parser.count_all

    rows_by_isin = {}
    for row in parse_gpw_rows(html_page):
        rows_by_isin[row["isin"]] = row

    # GPW's endpoint is most reliable with the same small pages used by the UI.
    limit = 50
    max_expected = expected_count or 1000
    for offset in range(0, max_expected + limit, limit):
        fields = []
        for key, value in base_fields:
            if key == "offset":
                value = str(offset)
            elif key == "limit":
                value = str(limit)
            fields.append((key, value))

        rows = []
        expected_page_count = min(limit, max(expected_count - offset, 1)) if expected_count else limit
        for _attempt in range(3):
            fragment = _request(GPW_AJAX_URL, fields)
            current_rows = parse_gpw_rows(fragment)
            if len(current_rows) > len(rows):
                rows = current_rows
            if len(rows) >= expected_page_count:
                break

        if not rows and offset > 0:
            break
        for row in rows:
            rows_by_isin[row["isin"]] = row
        if expected_count and len(rows_by_isin) >= expected_count:
            break

    return list(rows_by_isin.values()), expected_count


def import_gpw(args):
    conn = sqlite3.connect(args.db_path)
    job_id = None
    fetched = inserted = updated = 0
    try:
        ensure_schema(conn)
        upsert_source(conn)
        exchange_id = upsert_exchange(conn)
        job_id = start_job(conn, exchange_id)
        conn.commit()

        rows, expected_count = fetch_gpw_companies()
        fetched = len(rows)

        conn.execute("BEGIN")
        for row in rows:
            if upsert_company(conn, exchange_id, row):
                inserted += 1
            else:
                updated += 1
        finish_job(conn, job_id, "ok", fetched, inserted, updated)
        conn.commit()
        return {
            "source": GPW_SOURCE_CODE,
            "exchange": "GPW_MAIN",
            "expected_count": expected_count,
            "fetched": fetched,
            "inserted": inserted,
            "updated": updated,
        }
    except Exception as exc:
        conn.rollback()
        if job_id is not None:
            finish_job(conn, job_id, "error", fetched, inserted, updated, str(exc))
            conn.commit()
        return {"error": str(exc), "source": GPW_SOURCE_CODE}
    finally:
        conn.close()


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)

    gpw = sub.add_parser("import-gpw")
    gpw.add_argument("db_path")

    args = parser.parse_args()
    if args.command == "import-gpw":
        result = import_gpw(args)
    else:
        result = {"error": "Nieznane polecenie."}

    sys.stdout.write(json.dumps(result, ensure_ascii=False) + "\n")


if __name__ == "__main__":
    main()
