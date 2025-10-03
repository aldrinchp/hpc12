#!/usr/bin/env bash
set -euo pipefail

BIN="./output/reductions"
OUT="./output/reductions.csv"

# Tamaño y repeticiones: ajusta para ~0.5–2 s por prueba
N=${N:-40000000}
REPEATS=${REPEATS:-5}

export OMP_PROC_BIND=TRUE
export OMP_PLACES=cores

if [[ ! -x "$BIN" ]]; then
  echo "ERROR: $BIN not found/executable"
  exit 1
fi

echo "Mode,Threads,N,Time,Sum,Correct,Speedup,Efficiency" > "$OUT"

for MODE in critical atomic reduction; do
  base_time=""
  for T in 1 2 4 8 16 32; do
    export OMP_NUM_THREADS=$T
    LINE="$("$BIN" "$MODE" "$N" "$REPEATS")"     # Mode,Threads,N,Time,Sum,Correct
    IFS=',' read -r m th n tt sum ok <<< "$LINE"

    # baseline por modo: T=1
    if [[ "$T" -eq 1 ]]; then
      base_time="$tt"
    fi

    # Speedup y Efficiency
    SP=$(python3 - <<PY
base=$base_time
tt=$tt
print(base/tt if tt>0 else 0.0)
PY
)
    EF=$(python3 - <<PY
sp=$SP
t=$th
print(sp/t if t>0 else 0.0)
PY
)

    printf "%s,%s,%s,%.6f,%.17g,%s,%.6f,%.6f\n" "$m" "$th" "$n" "$tt" "$sum" "$ok" "$SP" "$EF" >> "$OUT"
    echo "$MODE  T=$T   time=${tt}s  S=${SP}  E=${EF}  correct=$ok"
  done
done

echo "OK -> $OUT"
