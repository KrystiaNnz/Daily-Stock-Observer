"""
edgar.py — pobiera filingi z SEC EDGAR dla danego tickera
Wejście:  python edgar.py <ticker> [form_type] [limit]
Wyjście:  JSON array na stdout

Przykład: python edgar.py AAPL 10-K 5
"""

import sys
import json
import urllib.request
import urllib.error

USER_AGENT = "DailyStockObserver krystiannicewicz777@gmail.com"

HEADERS_SEC  = {"User-Agent": USER_AGENT}
HEADERS_DATA = {"User-Agent": USER_AGENT, "Accept-Encoding": "gzip, deflate"}


def get_json(url: str, headers: dict) -> dict | list:
    req = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(req, timeout=20) as r:
        raw = r.read()
        # urlopen może zwrócić gzip — decode obsłuży utf-8
        return json.loads(raw.decode("utf-8"))


def find_cik(ticker: str) -> tuple[str, str]:
    """Zwraca (cik_padded_10, company_name) lub rzuca ValueError."""
    data = get_json("https://www.sec.gov/files/company_tickers.json", HEADERS_SEC)
    upper = ticker.upper()
    for entry in data.values():
        if entry.get("ticker", "").upper() == upper:
            cik = str(entry["cik_str"]).zfill(10)
            return cik, entry.get("title", ticker)
    raise ValueError(f"Nie znaleziono CIK dla tickera: {ticker}")


def fetch_filings(ticker: str, form_type: str = "10-K", limit: int = 10) -> list:
    cik, company = find_cik(ticker)

    url  = f"https://data.sec.gov/submissions/CIK{cik}.json"
    data = get_json(url, HEADERS_DATA)

    recent       = data.get("filings", {}).get("recent", {})
    forms        = recent.get("form",                [])
    filing_dates = recent.get("filingDate",          [])
    report_dates = recent.get("reportDate",          [])
    accessions   = recent.get("accessionNumber",     [])
    primary_docs = recent.get("primaryDocument",     [])
    descriptions = recent.get("primaryDocDescription", [])

    results = []
    for i, form in enumerate(forms):
        if form_type and form != form_type:
            continue

        acc_clean = accessions[i].replace("-", "") if i < len(accessions) else ""
        doc       = primary_docs[i]  if i < len(primary_docs)  else ""
        desc      = descriptions[i] if i < len(descriptions) else ""
        acc_raw   = accessions[i]   if i < len(accessions)    else ""

        filing_url = (
            f"https://www.sec.gov/Archives/edgar/data/{int(cik)}/{acc_clean}/{doc}"
            if acc_clean and doc else ""
        )

        results.append({
            "ticker":      ticker.upper(),
            "company":     company,
            "cik":         cik,
            "form":        form,
            "filingDate":  filing_dates[i] if i < len(filing_dates) else "",
            "reportDate":  report_dates[i] if i < len(report_dates) else "",
            "accession":   acc_raw,
            "document":    doc,
            "description": desc,
            "url":         filing_url,
        })

        if len(results) >= limit:
            break

    return results


if __name__ == "__main__":
    if len(sys.argv) < 2 or not sys.argv[1].strip():
        print(json.dumps({"error": "Użycie: edgar.py <ticker> [form_type] [limit]"}))
        sys.exit(1)

    ticker    = sys.argv[1].strip()
    form_type = sys.argv[2].strip() if len(sys.argv) > 2 else "10-K"
    limit     = int(sys.argv[3])    if len(sys.argv) > 3 else 10

    try:
        filings = fetch_filings(ticker, form_type, limit)
        print(json.dumps(filings, ensure_ascii=False))
    except urllib.error.HTTPError as e:
        print(json.dumps({"error": f"HTTP {e.code}: {e.reason}"}))
        sys.exit(1)
    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.exit(1)
