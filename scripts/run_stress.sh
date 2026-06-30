#!/usr/bin/env bash

set -euo pipefail

SIMULATOR="./bin/vmsim"
TRACE_GENERATOR="./bin/tracegen"

SEQUENTIAL_TRACE="traces/stress_sequential.trace"
RANDOM_TRACE="traces/stress_random.trace"

OUTPUT="resultados/stress.csv"

PAGES=64
ACCESSES=20000
WRITE_RATE=25
SEED=2026

TLB_ENTRIES=16
SWAP_SLOTS=256

POLICIES=(
    fifo
    lru
    clock
    aging
)

FRAME_COUNTS=(
    8
    16
)

if [[ ! -x "$SIMULATOR" ]]; then
    echo "Erro: executável $SIMULATOR não encontrado."
    echo "Execute 'make' antes do teste."
    exit 1
fi

if [[ ! -x "$TRACE_GENERATOR" ]]; then
    echo "Erro: executável $TRACE_GENERATOR não encontrado."
    echo "Execute 'make' antes do teste."
    exit 1
fi

mkdir -p traces resultados bin

echo "Gerando trace sequencial..."

"$TRACE_GENERATOR" \
    --output "$SEQUENTIAL_TRACE" \
    --pattern sequential \
    --pages "$PAGES" \
    --accesses "$ACCESSES" \
    --write-rate "$WRITE_RATE" \
    --seed "$SEED" \
    > /dev/null

echo "Gerando trace aleatório..."

"$TRACE_GENERATOR" \
    --output "$RANDOM_TRACE" \
    --pattern random \
    --pages "$PAGES" \
    --accesses "$ACCESSES" \
    --write-rate "$WRITE_RATE" \
    --seed "$SEED" \
    > /dev/null

rm -f "$OUTPUT"

first_execution=true

run_simulation() {
    local trace="$1"
    local policy="$2"
    local frames="$3"

    local csv_options=()

    if [[ "$first_execution" == true ]]; then
        csv_options+=(--csv-overwrite)
        first_execution=false
    fi

    local trace_name
    trace_name="$(basename "$trace")"

    local swap_path
    swap_path="bin/stress_${trace_name}_${policy}_${frames}.swap"

    echo \
        "Trace: $trace_name | Política: $policy | Quadros: $frames"

    "$SIMULATOR" \
        --trace "$trace" \
        --frames "$frames" \
        --policy "$policy" \
        --tlb "$TLB_ENTRIES" \
        --swap "$swap_path" \
        --swap-slots "$SWAP_SLOTS" \
        --csv "$OUTPUT" \
        "${csv_options[@]}" \
        > /dev/null
}

for trace in \
    "$SEQUENTIAL_TRACE" \
    "$RANDOM_TRACE"
do
    for policy in "${POLICIES[@]}"; do
        for frames in "${FRAME_COUNTS[@]}"; do
            run_simulation \
                "$trace" \
                "$policy" \
                "$frames"
        done
    done
done

echo
echo "Execuções de estresse concluídas."
echo "Resultados: $OUTPUT"
echo

./scripts/check_stress.sh "$OUTPUT"