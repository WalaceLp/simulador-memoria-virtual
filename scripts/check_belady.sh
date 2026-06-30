#!/usr/bin/env bash

set -euo pipefail

CSV_PATH="${1:-resultados/belady.csv}"

if [[ ! -f "$CSV_PATH" ]]; then
    echo "Erro: arquivo $CSV_PATH não encontrado."
    exit 1
fi

get_page_faults() {
    local policy="$1"
    local frames="$2"

    awk -F',' \
        -v target_policy="\"$policy\"" \
        -v target_frames="$frames" '
        NR > 1 {
            current_policy = $4
            current_frames = $3
            page_faults = $11

            if (current_policy == target_policy && current_frames == target_frames) {
                gsub(/"/, "", page_faults)
                print page_faults
                exit
            }
        }
    ' "$CSV_PATH"
}

fifo_3="$(get_page_faults FIFO 3)"
fifo_4="$(get_page_faults FIFO 4)"
lru_3="$(get_page_faults LRU 3)"
lru_4="$(get_page_faults LRU 4)"

if [[ -z "$fifo_3" || -z "$fifo_4" ]]; then
    echo "Erro: resultados do FIFO não encontrados."
    exit 1
fi

if [[ -z "$lru_3" || -z "$lru_4" ]]; then
    echo "Erro: resultados do LRU não encontrados."
    exit 1
fi

echo "=== Resultado do experimento ==="

printf "%-10s %-10s %-12s\n" \
    "Política" \
    "Quadros" \
    "Page faults"

printf "%-10s %-10s %-12s\n" \
    "FIFO" \
    "3" \
    "$fifo_3"

printf "%-10s %-10s %-12s\n" \
    "FIFO" \
    "4" \
    "$fifo_4"

printf "%-10s %-10s %-12s\n" \
    "LRU" \
    "3" \
    "$lru_3"

printf "%-10s %-10s %-12s\n" \
    "LRU" \
    "4" \
    "$lru_4"

echo

if (( fifo_4 > fifo_3 )); then
    echo "Anomalia de Belady confirmada no FIFO:"
    echo "$fifo_3 page faults com 3 quadros."
    echo "$fifo_4 page faults com 4 quadros."
else
    echo "Erro: a anomalia de Belady não foi observada."
    exit 1
fi

if (( lru_4 <= lru_3 )); then
    echo
    echo "LRU não apresentou a anomalia:"
    echo "$lru_3 page faults com 3 quadros."
    echo "$lru_4 page faults com 4 quadros."
else
    echo
    echo "Erro: resultado inesperado para o LRU."
    exit 1
fi

if [[ "$fifo_3" -ne 9 ]]; then
    echo
    echo "Erro: FIFO com 3 quadros deveria produzir 9 page faults."
    echo "Resultado obtido: $fifo_3"
    exit 1
fi

if [[ "$fifo_4" -ne 10 ]]; then
    echo
    echo "Erro: FIFO com 4 quadros deveria produzir 10 page faults."
    echo "Resultado obtido: $fifo_4"
    exit 1
fi

if [[ "$lru_3" -ne 10 ]]; then
    echo
    echo "Erro: LRU com 3 quadros deveria produzir 10 page faults."
    echo "Resultado obtido: $lru_3"
    exit 1
fi

if [[ "$lru_4" -ne 8 ]]; then
    echo
    echo "Erro: LRU com 4 quadros deveria produzir 8 page faults."
    echo "Resultado obtido: $lru_4"
    exit 1
fi

echo
echo "Os resultados correspondem aos valores esperados."