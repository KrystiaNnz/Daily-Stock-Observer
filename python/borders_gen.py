#!/usr/bin/env python3
"""
borders_gen.py — pobiera Natural Earth 110m GeoJSON i generuje kompaktowy
plik borders_data.json zawierający pierścienie granic państw z kodami ISO.
Format wyjściowy: [{"iso":"PL","coords":[[lon,lat],...]},...]}
Uruchomienie: python borders_gen.py [ścieżka_wyjściowa]
"""
import json, sys, urllib.request

URL = ("https://raw.githubusercontent.com/nvkelso/natural-earth-vector"
       "/master/geojson/ne_110m_admin_0_countries.geojson")

print("Pobieranie Natural Earth 110m countries...", file=sys.stderr)
with urllib.request.urlopen(URL, timeout=30) as f:
    data = json.load(f)

rings = []
for feature in data["features"]:
    props = feature["properties"]
    iso   = props.get("ISO_A2", "-99").strip()
    if iso == "-99" or iso == "":
        # fallback: pierwsze dwa znaki ADM0_A3
        a3  = props.get("ADM0_A3", "")
        iso = a3[:2] if len(a3) >= 2 else "??"
    iso = iso.upper()

    geom = feature["geometry"]
    polys = [geom["coordinates"]] if geom["type"] == "Polygon" else geom["coordinates"]

    for poly in polys:
        exterior   = poly[0]
        simplified = [[round(p[0], 2), round(p[1], 2)] for p in exterior]
        if len(simplified) >= 3:
            rings.append({"iso": iso, "coords": simplified})

out_path = sys.argv[1] if len(sys.argv) > 1 else "borders_data.json"
with open(out_path, "w", encoding="utf-8") as f:
    json.dump(rings, f, separators=(",", ":"))

print(f"Wygenerowano {len(rings)} pierścieni granic -> {out_path}", file=sys.stderr)
