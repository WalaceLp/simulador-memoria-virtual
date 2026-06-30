#!/usr/bin/env bash

set -euo pipefail

echo "========================================"
echo "Validação final do simulador"
echo "========================================"
echo

echo "[1/6] Limpando arquivos compilados..."
make clean

echo
echo "[2/6] Compilando com avisos como erro..."
make

echo
echo "[3/6] Executando testes unitários e integrados..."
make test

echo
echo "[4/6] Executando experimento de Belady..."
make belady

echo
echo "[5/6] Executando testes de estresse..."
make stress

echo
echo "[6/6] Executando análise com Valgrind..."
make valgrind

echo
echo "========================================"
echo "Validação final concluída com sucesso."
echo "========================================"