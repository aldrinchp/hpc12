# scripts/benchmark_matmul.sh
#!/usr/bin/env bash
set -euo pipefail

BIN="./output/matmul"
OUT="./output/matmul.csv"

# Tamaños y repeticiones (ajusta a tu máquina)
SIZES=${SIZES:-"512 768 1024"}
REPEATS=${REPEATS:-3}
BS=${BS:-64}

export OMP_PROC_BIND=TRUE
export OMP_PLACES=cores

if [[ ! -x "$BIN" ]]; then
  echo "ERROR: $BIN not found"
  exit 1
fi

echo "Variant,Threads,n,BS,Time,GFLOPs,RelError,Speedup,Efficiency" > "$OUT"

for V in single collapse blocked; do
  for n in $SIZES; do
    base_time=""
    for T in 1 2 4 8 16 32; do
      export OMP_NUM_THREADS=$T
      LINE="$("$BIN" "$V" "$n" "$BS" "$REPEATS")"   # CSV: Variant,Threads,n,BS,Time,GFLOPs,RelError
      IFS=',' read -r variant threads n_ bs time gflops rel <<< "$LINE"
      if [[ "$T" -eq 1 ]]; then base_time="$time"; fi

      # speedup/eff
      SP=$(python3 - <<PY
base=$base_time; tt=$time
print(base/tt if tt>0 else 0.0)
PY
)
      EF=$(python3 - <<PY
sp=$SP; t=$threads
print(sp/t if t>0 else 0.0)
PY
)

      printf "%s,%s,%s,%s,%.6f,%.6f,%.3e,%.6f,%.6f\n" \
        "$variant" "$threads" "$n_" "$bs" "$time" "$gflops" "$rel" "$SP" "$EF" >> "$OUT"

      echo "$variant n=$n T=$T  time=$time s  GFLOPs=$gflops  S=$SP  E=$EF  relErr=$rel"
    done
  done
done

echo "OK -> $OUT"
