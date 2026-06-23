#!/usr/bin/env python3
"""
regions_gen.py — generuje regions_data.json z podziałem per kraj.
Źródła:
  • Eurostat NUTS-2 (EU+EEA) — pokrywa wszystkie kraje europejskie
  • Natural Earth 50m admin-1 — USA, Rosja, Brazylia, Chiny, Kanada, itp.
Format wyjściowy: {"PL":[[[lon,lat],...],...],"DE":[...],...}
Uruchomienie: python regions_gen.py [ścieżka_wyjściowa]
"""
import json, sys, urllib.request

# Mapowanie specjalnych kodów NUTS -> ISO-2
NUTS_TO_ISO = {
    "EL": "GR",   # Grecja
    "UK": "GB",   # Wielka Brytania (historyczny kod NUTS)
}

def fetch(url, label):
    print(f"Pobieranie {label}...", file=sys.stderr)
    req = urllib.request.Request(url, headers={"User-Agent": "PlanerDnia/1.0"})
    with urllib.request.urlopen(req, timeout=60) as f:
        return json.load(f)

def extract_rings(geom):
    """Zwraca listę uproszczonych pierścieni zewnętrznych z geometrii."""
    if not geom:
        return []
    polys = ([geom["coordinates"]]
             if geom["type"] == "Polygon"
             else geom["coordinates"])
    rings = []
    for poly in polys:
        exterior = poly[0]
        simplified = [[round(p[0], 2), round(p[1], 2)] for p in exterior]
        if len(simplified) >= 3:
            rings.append(simplified)
    return rings

regions = {}   # iso2 -> [[lon,lat],...]

# ── 1. Eurostat NUTS-2 (Europa + kraje EOG) ─────────────────────────────────
URL_NUTS = (
    "https://gisco-services.ec.europa.eu/distribution/v2/nuts/geojson/"
    "NUTS_RG_20M_2021_4326_LEVL_2.geojson"
)
nuts_data = fetch(URL_NUTS, "Eurostat NUTS-2")
for feat in nuts_data["features"]:
    props = feat.get("properties", {})
    cntr  = props.get("CNTR_CODE", "").strip().upper()
    iso2  = NUTS_TO_ISO.get(cntr, cntr)
    if len(iso2) != 2:
        continue
    for ring in extract_rings(feat.get("geometry")):
        regions.setdefault(iso2, []).append(ring)

print(f"  Eurostat: {len(regions)} krajów europejskich.", file=sys.stderr)

# ── 2. Natural Earth 50m admin-1 (duże kraje spoza Europy) ──────────────────
URL_NE = (
    "https://raw.githubusercontent.com/nvkelso/natural-earth-vector"
    "/master/geojson/ne_50m_admin_1_states_provinces.geojson"
)
ne_data = fetch(URL_NE, "Natural Earth 50m admin-1 (non-EU)")
for feat in ne_data["features"]:
    props = feat.get("properties", {})
    iso2  = props.get("iso_a2", "").strip().upper()
    if len(iso2) != 2 or iso2 == "-9":
        continue
    if iso2 in regions:          # już mamy z Eurostatu — pomijamy
        continue
    for ring in extract_rings(feat.get("geometry")):
        regions.setdefault(iso2, []).append(ring)

total = sum(len(v) for v in regions.values())
print(f"Łącznie {len(regions)} krajów, {total} pierścieni.", file=sys.stderr)

for iso in ["PL", "DE", "FR", "GB", "US", "RU", "CN", "BR"]:
    n = len(regions.get(iso, []))
    print(f"  {iso}: {n} regionów", file=sys.stderr)

out_path = sys.argv[1] if len(sys.argv) > 1 else "regions_data.json"
with open(out_path, "w", encoding="utf-8") as f:
    json.dump(regions, f, separators=(",", ":"))
print(f"Zapisano -> {out_path}", file=sys.stderr)
