#!/bin/bash

EXEC=./output/uneven_workload
OUTPUT="heavy_results.csv"

echo "Schedule,Chunk,Threads,Time" > $OUTPUT

SCHEDULES=("static" "dynamic" "guided")
CHUNKS=(1 4 8 16 32)
THREADS=(1 2 4 8 16)

for sched in "${SCHEDULES[@]}"; do
  for chunk in "${CHUNKS[@]}"; do
    for t in "${THREADS[@]}"; do
      TIME=$($EXEC $sched $chunk $t | grep "Execution time" | awk '{print $3}')
      echo "$sched,$chunk,$t,$TIME" >> $OUTPUT
      echo "Done: $sched, chunk=$chunk, threads=$t -> time=$TIME"
    done
  done
done

echo "All results saved in $OUTPUT"
