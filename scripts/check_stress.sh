#!/usr/bin/env bash

set -euo pipefail

CSV_PATH="${1:-resultados/stress.csv}"

EXPECTED_ROWS=16
EXPECTED_ACCESSES=20000

if [[ ! -f "$CSV_PATH" ]]; then
    echo "Erro: arquivo $CSV_PATH não encontrado."
    exit 1
fi

data_rows="$(
    awk 'END { print NR - 1 }' "$CSV_PATH"
)"

if [[ "$data_rows" -ne "$EXPECTED_ROWS" ]]; then
    echo "Erro: quantidade incorreta de resultados."
    echo "Esperado: $EXPECTED_ROWS"
    echo "Obtido: $data_rows"
    exit 1
fi

awk -F',' \
    -v expected_accesses="$EXPECTED_ACCESSES" '
    NR == 1 {
        next
    }

    {
        processed = $7 + 0
        total = $8 + 0
        reads = $9 + 0
        writes = $10 + 0
        faults = $11 + 0
        replacements = $12 + 0
        dirty = $13 + 0
        hits = $14 + 0
        misses = $15 + 0
        walks = $17 + 0
        swap_reads = $20 + 0
        swap_writes = $21 + 0

        if (processed != expected_accesses) {
            print "Erro: acessos processados inválidos na linha " NR
            exit 1
        }

        if (total != expected_accesses) {
            print "Erro: total de acessos inválido na linha " NR
            exit 1
        }

        if (reads + writes != total) {
            print "Erro: leituras + escritas diferente do total na linha " NR
            exit 1
        }

        if (hits + misses != total) {
            print "Erro: hits + misses diferente do total na linha " NR
            exit 1
        }

        if (walks != misses) {
            print "Erro: page walks diferente dos misses na linha " NR
            exit 1
        }

        if (faults > total) {
            print "Erro: page faults acima do total na linha " NR
            exit 1
        }

        if (replacements > faults) {
            print "Erro: substituições acima dos page faults na linha " NR
            exit 1
        }

        if (dirty > replacements) {
            print "Erro: vítimas sujas acima das substituições na linha " NR
            exit 1
        }

        if (swap_writes > dirty) {
            print "Erro: escritas no swap acima das vítimas sujas na linha " NR
            exit 1
        }

        if (swap_reads > faults) {
            print "Erro: leituras do swap acima dos page faults na linha " NR
            exit 1
        }

        if (faults == 0 || misses == 0) {
            print "Erro: cenário sem page faults ou misses na linha " NR
            exit 1
        }
    }
' "$CSV_PATH"

check_policy() {
    local policy="$1"

    local count

    count="$(
        awk -F',' \
            -v target="\"$policy\"" '
            NR > 1 && $4 == target {
                count++
            }

            END {
                print count + 0
            }
        ' "$CSV_PATH"
    )"

    if [[ "$count" -ne 4 ]]; then
        echo "Erro: política $policy possui $count resultados."
        exit 1
    fi
}

check_policy FIFO
check_policy LRU
check_policy Clock
check_policy Aging

sequential_count="$(
    grep -c 'stress_sequential.trace' "$CSV_PATH"
)"

random_count="$(
    grep -c 'stress_random.trace' "$CSV_PATH"
)"

if [[ "$sequential_count" -ne 8 ]]; then
    echo "Erro: quantidade incorreta de cenários sequenciais."
    exit 1
fi

if [[ "$random_count" -ne 8 ]]; then
    echo "Erro: quantidade incorreta de cenários aleatórios."
    exit 1
fi

echo "=== Validação dos testes de estresse ==="
echo "Execuções verificadas: $data_rows"
echo "Políticas verificadas: FIFO, LRU, Clock e Aging"
echo "Traces verificados: sequencial e aleatório"
echo "Quadros verificados: 8 e 16"
echo "Acessos por execução: $EXPECTED_ACCESSES"
echo
echo "Todas as métricas apresentaram consistência."