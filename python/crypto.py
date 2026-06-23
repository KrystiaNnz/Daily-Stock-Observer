"""
crypto.py — szyfrowanie/deszyfrowanie tekstu
Używany przez Daily Stock Observer do szyfrowania opisów wydarzeń.

Użycie:
  python crypto.py encrypt "tekst do zaszyfrowania"
  python crypto.py decrypt "zaszyfrowany_tekst"

Klucz szyfrowania jest generowany raz i zapisywany w pliku .key
obok bazy danych (AppData/Roaming/DailyStockObserver/).
"""

import sys
import os
from pathlib import Path

# Instalacja: pip install cryptography
try:
    from cryptography.fernet import Fernet
except ImportError:
    print("ERROR: brakuje biblioteki. Uruchom: pip install cryptography", file=sys.stderr)
    sys.exit(1)

KEY_FILE = Path(os.environ.get("APPDATA", ".")) / "DailyStockObserver" / "secret.key"

def load_or_create_key() -> bytes:
    """Wczytaj klucz z pliku lub wygeneruj nowy."""
    KEY_FILE.parent.mkdir(parents=True, exist_ok=True)
    if KEY_FILE.exists():
        return KEY_FILE.read_bytes()
    key = Fernet.generate_key()
    KEY_FILE.write_bytes(key)
    return key

def encrypt(text: str) -> str:
    key = load_or_create_key()
    f = Fernet(key)
    return f.encrypt(text.encode()).decode()

def decrypt(token: str) -> str:
    key = load_or_create_key()
    f = Fernet(key)
    return f.decrypt(token.encode()).decode()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Użycie: crypto.py [encrypt|decrypt] <tekst>", file=sys.stderr)
        sys.exit(1)

    action, text = sys.argv[1], sys.argv[2]

    if action == "encrypt":
        print(encrypt(text))
    elif action == "decrypt":
        print(decrypt(text))
    else:
        print(f"Nieznana akcja: {action}", file=sys.stderr)
        sys.exit(1)
