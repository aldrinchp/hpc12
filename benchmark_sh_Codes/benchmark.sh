#!/bin/bash

EXEC=./output/omp_version   # tu ejecutable compilado con -fopenmp
N=16000000                      # tamaño del problema
OUTFILE="tiempos.csv"

# Estrategias y chunk sizes que quieres probar
SCHEDULES=("static" "dynamic" "guided")
CHUNKS=(1 4 8 16 32)

echo "Schedule,Chunk,Threads,VectorAdd,MatrixMultiply" > $OUTFILE

for sched in "${SCHEDULES[@]}"; do
  for chunk in "${CHUNKS[@]}"; do
    for t in 1 2 4 8 16 32; do
      echo "Ejecutando con $t hilos, schedule=$sched, chunk=$chunk..."
      export OMP_NUM_THREADS=$t
      export OMP_SCHEDULE=$sched,$chunk
      
      OUTPUT=$($EXEC $N)
      
      VEC=$(echo "$OUTPUT" | grep "suma de vectores" | awk '{print $(NF-1)}')
      MAT=$(echo "$OUTPUT" | grep "multiplicación de matrices" | awk '{print $(NF-1)}')
      
      echo "$sched,$chunk,$t,$VEC,$MAT" >> $OUTFILE
    done
  done
done

echo "Resultados guardados en $OUTFILE"