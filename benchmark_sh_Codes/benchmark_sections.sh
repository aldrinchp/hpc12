#!/usr/bin/env bash
set -euo pipefail

BIN="./output/sections"
OUT="./output/sections.csv"

# Ajusta la carga de trabajo: sube/baja para que cada corrida dure ~0.5-3 s
ITERS=${ITRS:-20000000}

# Afinidad (opcional pero recomendable para estabilidad de medidas)
export OMP_PROC_BIND=TRUE
export OMP_PLACES=cores

if [[ ! -x "$BIN" ]]; then
  echo "ERROR: binary $BIN not found or not executable"
  exit 1
fi

# Header CSV
echo "Sections,Threads,Work,Time,Speedup,Efficiency" > "$OUT"

for SECS in 2 3 4; do
  BASE_T=""
  for T in 1 2 3 4; do
    export OMP_NUM_THREADS=$T
    LINE="$("$BIN" "$SECS" "$ITERS")"   # "Sections,Threads,Work,Time"
    # Parseo robusto:
    # Ejemplo: LINE="3,2,20000000,0.812345"
    IFS=',' read -r S TH W TT <<< "$LINE"

    if [[ "$T" -eq 1 ]]; then
      BASE_T="$TT"
    fi

    # Speedup y Efficiency con bc (coma flotante)
    SP=$(python3 - <<PY
base=$BASE_T
tt=$TT
print(base/tt if tt>0 else 0.0)
PY
)
    EF=$(python3 - <<PY
sp=$SP
t=$TH
print(sp/t if t>0 else 0.0)
PY
)

    printf "%s,%s,%s,%.6f,%.6f,%.6f\n" "$S" "$TH" "$W" "$TT" "$SP" "$EF" >> "$OUT"
    echo "SECTIONS=$S THREADS=$TH  Time=${TT}s  Speedup=${SP}  Eff=${EF}"
  done
done

echo "OK -> $OUT"
