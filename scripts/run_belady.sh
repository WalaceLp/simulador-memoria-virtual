#!/usr/bin/env bash

set -euo pipefail

SIMULATOR="./bin/vmsim"
TRACE="traces/belady.trace"
OUTPUT="resultados/belady.csv"

TLB_ENTRIES=1
SWAP_SLOTS=32

if [[ ! -x "$SIMULATOR" ]]; then
    echo "Erro: executável $SIMULATOR não encontrado."
    echo "Execute 'make' antes do experimento."
    exit 1
fi

if [[ ! -f "$TRACE" ]]; then
    echo "Erro: trace $TRACE não encontrado."
    exit 1
fi

mkdir -p resultados

rm -f "$OUTPUT"

echo "Executando experimento da anomalia de Belady..."
echo

run_simulation() {
    local policy="$1"
    local frames="$2"
    local csv_mode="$3"

    local csv_option=()

    if [[ "$csv_mode" == "overwrite" ]]; then
        csv_option+=(--csv-overwrite)
    fi

    echo "Política: $policy | Quadros: $frames"

    "$SIMULATOR" \
        --trace "$TRACE" \
        --frames "$frames" \
        --policy "$policy" \
        --tlb "$TLB_ENTRIES" \
        --swap-slots "$SWAP_SLOTS" \
        --csv "$OUTPUT" \
        "${csv_option[@]}" \
        > /dev/null
}

run_simulation fifo 3 overwrite
run_simulation fifo 4 append
run_simulation lru 3 append
run_simulation lru 4 append

echo
echo "Experimento concluído."
echo "Resultados salvos em: $OUTPUT"
echo

./scripts/check_belady.sh "$OUTPUT"