"""
analysis.py — analiza danych z bazy wydarzeń Daily Stock Observer
Wyniki zwracane jako JSON na stdout — Qt je odczytuje i wyświetla.

Użycie:
  python analysis.py <ścieżka_do_daily_stock_observer.db>

Instalacja: pip install pandas
"""

import sys
import json
import sqlite3
from pathlib import Path
from datetime import datetime, timedelta
from collections import Counter

try:
    import pandas as pd
except ImportError:
    print(json.dumps({"error": "Brakuje biblioteki pandas. Uruchom: pip install pandas"}))
    sys.exit(1)

def analyze(db_path: str) -> dict:
    conn = sqlite3.connect(db_path)
    df = pd.read_sql_query(
        "SELECT id, title, date, time, description FROM events", conn)
    conn.close()

    if df.empty:
        return {"info": "Brak wydarzeń w bazie danych."}

    df["date"] = pd.to_datetime(df["date"])

    # Statystyki ogólne
    total = len(df)
    oldest = df["date"].min().strftime("%d.%m.%Y")
    newest = df["date"].max().strftime("%d.%m.%Y")

    # Najbardziej zapełnione dni
    busiest = df.groupby("date").size().nlargest(3)
    busiest_list = [
        {"date": d.strftime("%d.%m.%Y"), "count": int(c)}
        for d, c in busiest.items()
    ]

    # Liczba wydarzeń wg dnia tygodnia
    df["weekday"] = df["date"].dt.day_name()
    weekday_counts = df["weekday"].value_counts().to_dict()

    # Ostatnie 7 dni
    today = pd.Timestamp(datetime.today().date())
    week_ago = today - timedelta(days=7)
    last_week = int((df["date"] >= week_ago).sum())

    return {
        "total_events": total,
        "date_range": {"from": oldest, "to": newest},
        "last_7_days": last_week,
        "busiest_days": busiest_list,
        "by_weekday": weekday_counts,
    }

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(json.dumps({"error": "Użycie: analysis.py <ścieżka_do_daily_stock_observer.db>"}))
        sys.exit(1)

    db_path = sys.argv[1]
    if not Path(db_path).exists():
        print(json.dumps({"error": f"Nie znaleziono pliku: {db_path}"}))
        sys.exit(1)

    result = analyze(db_path)
    print(json.dumps(result, ensure_ascii=False, indent=2))
