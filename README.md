# Simulador de Memoria Virtual

Projeto interdisciplinar de Estrutura de Dados e Sistemas Operacionais do Ifes
Cachoeiro para o **Tema 3 - Tabela de Paginas Multinivel com Trie de Traducao
de Enderecos**.

O sistema simula uma memoria virtual dirigida por traces de acesso. A estrutura
central e uma trie/radix tree de quatro niveis, inspirada no page walk de
arquiteturas x86-64. Sobre ela o simulador integra TLB, page faults, algoritmos
de substituicao, memoria fisica, swap em arquivo binario, fork com
copy-on-write e exportacao de metricas em CSV.

## Requisitos de ambiente

- GCC com suporte a C11
- GNU Make
- Bash
- Valgrind, para o alvo `make valgrind`

O projeto e compilado com:

```sh
gcc -std=c11 -Wall -Wextra -Werror
```

## Estrutura do repositorio

```text
include/       Cabecalhos publicos dos modulos
src/           Implementacao do simulador e das politicas
tests/         Testes unitarios e integrados
scripts/       Scripts de validacao, Belady e estresse
traces/        Traces de exemplo, Belady e estresse
resultados/    CSVs gerados pelos experimentos
DIARIO.md      Diario de engenharia e rastreabilidade de IA
Makefile       Build, testes e validacoes
```

Principais modulos:

- `address`: decomposicao de enderecos virtuais.
- `page_table`: trie multinivel da tabela de paginas.
- `process`: criacao, destruicao e fork de processos.
- `physical_memory`: quadros fisicos e conteudo real das paginas.
- `replacement`: interface comum para FIFO, LRU, Clock e Aging.
- `tlb`: cache de traducoes por PID e pagina virtual.
- `swap`: arquivo binario dividido em slots de 4096 bytes.
- `virtual_memory`: integracao de page fault, TLB, swap, COW e metricas.
- `trace`: leitura e reproducao de traces.
- `trace_generator`: geracao de traces sequenciais e aleatorios.
- `csv_export`: exportacao das metricas por execucao.

## Modelo de endereco

O simulador usa constantes equivalentes a uma tabela de paginas multinivel:

- tamanho da pagina: 4096 bytes;
- bits de deslocamento: 12;
- niveis da trie: 4;
- bits de indice por nivel: 9;
- entradas por nivel: 512.

Cada endereco virtual e decomposto em:

```text
[ indice nivel 0 ][ indice nivel 1 ][ indice nivel 2 ][ indice nivel 3 ][ offset ]
       9 bits           9 bits           9 bits           9 bits        12 bits
```

Os subniveis da trie sao criados sob demanda. Ao remover mapeamentos, subarvores
vazias sao podadas.

## Compilacao

```sh
make
```

Esse comando gera:

- `bin/vmsim`: simulador principal;
- `bin/tracegen`: gerador de traces.

Para limpar artefatos compilados:

```sh
make clean
```

## Execucao do simulador

Uso basico:

```sh
./bin/vmsim --trace traces/exemplo.trace
```

Exemplo com parametros explicitos:

```sh
./bin/vmsim \
  --trace traces/belady.trace \
  --frames 3 \
  --policy fifo \
  --tlb 1 \
  --swap bin/vmsim.swap \
  --swap-slots 32 \
  --pid 1 \
  --csv resultados/execucao.csv \
  --csv-overwrite
```

Opcoes aceitas:

- `--trace <arquivo>`: trace de entrada.
- `--frames <quantidade>`: quantidade de quadros fisicos.
- `--policy <politica>`: `fifo`, `lru`, `clock` ou `aging`.
- `--tlb <quantidade>`: quantidade de entradas da TLB.
- `--swap <arquivo>`: caminho do arquivo de swap.
- `--swap-slots <numero>`: quantidade de slots do swap.
- `--pid <numero>`: PID do processo simulado.
- `--csv <arquivo>`: arquivo CSV de saida.
- `--csv-overwrite`: sobrescreve o CSV em vez de acrescentar.
- `--keep-swap`: preserva o arquivo de swap ao final.
- `--help`: mostra a ajuda.

## Formato dos traces

O leitor aceita um formato simplificado:

```text
R 0x1000
W 0x2000
```

Tambem aceita operacoes no estilo Valgrind/Lackey:

- `I`: instrucao, ignorada;
- `L`: leitura;
- `S`: escrita;
- `M`: leitura seguida de escrita;
- `R`: leitura, formato simplificado;
- `W`: escrita, formato simplificado.

Linhas vazias e comentarios iniciados por `#` sao ignorados. Acessos com tamanho
que cruzam paginas sao reproduzidos como um acesso por pagina tocada.

## Geracao de traces

Gerar trace sequencial:

```sh
./bin/tracegen \
  --output traces/meu_sequencial.trace \
  --pattern sequential \
  --pages 64 \
  --accesses 20000 \
  --write-rate 25 \
  --seed 2026
```

Gerar trace aleatorio:

```sh
./bin/tracegen \
  --output traces/meu_aleatorio.trace \
  --pattern random \
  --pages 64 \
  --accesses 20000 \
  --write-rate 25 \
  --seed 2026
```

Opcoes do gerador:

- `--output <arquivo>`: caminho do trace gerado.
- `--pattern <padrao>`: `sequential` ou `random`.
- `--pages <quantidade>`: paginas virtuais distintas.
- `--accesses <quantidade>`: quantidade de acessos.
- `--write-rate <0-100>`: percentual de escritas.
- `--seed <numero>`: semente para reproducibilidade.
- `--base <endereco>`: endereco base.

## Testes e validacao

Executar todos os testes unitarios e integrados:

```sh
make test
```

Executar o experimento da anomalia de Belady:

```sh
make belady
```

Valores esperados:

| Politica | Quadros | Page faults |
|---|---:|---:|
| FIFO | 3 | 9 |
| FIFO | 4 | 10 |
| LRU | 3 | 10 |
| LRU | 4 | 8 |

Executar testes de estresse:

```sh
make stress
```

O estresse gera traces sequencial e aleatorio, executa as quatro politicas
(`fifo`, `lru`, `clock`, `aging`) com 8 e 16 quadros e valida a consistencia das
metricas exportadas.

Executar Valgrind nos testes:

```sh
make valgrind
```

Executar a validacao final completa:

```sh
make validate
```

Esse alvo executa, em sequencia:

1. `make clean`
2. `make`
3. `make test`
4. `make belady`
5. `make stress`
6. `make valgrind`

## Metricas exportadas

Quando `--csv` e usado, o simulador grava uma linha por execucao com:

- trace;
- PID;
- quadros fisicos;
- politica de substituicao;
- entradas da TLB;
- slots de swap;
- acessos processados;
- leituras e escritas;
- page faults;
- substituicoes;
- vitimas sujas;
- TLB hits e misses;
- taxa de acerto da TLB;
- page walks;
- niveis percorridos;
- media de niveis por page walk;
- leituras e escritas no swap.

Os resultados dos experimentos principais ficam em:

- `resultados/belady.csv`;
- `resultados/stress.csv`.

## Funcionalidades implementadas

- Tabela de paginas multinivel como trie/radix tree.
- Page walk com contagem de niveis percorridos.
- Mapeamento, remapeamento, busca e remocao de paginas.
- Criacao preguicosa de subniveis e poda de subarvores vazias.
- Processos com PID e tabela de paginas propria.
- Fork com compartilhamento estrutural inicial da trie.
- Copy-on-write fisico quando existe quadro livre para a copia.
- Memoria fisica com quadros ocupados/livres, PID, pagina virtual, bits de
  referencia/modificacao e conteudo real da pagina.
- TLB por chave `(PID, pagina virtual)`.
- Invalidacao individual e por processo na TLB.
- Page faults e substituicao de paginas.
- Politicas FIFO, LRU, Clock e Aging.
- Swap binario em slots de 4096 bytes.
- Escrita no swap apenas de paginas sujas.
- Recuperacao de paginas sujas via swap-in.
- Parser de traces simplificados e Lackey.
- Gerador de traces sequenciais e aleatorios.
- Exportacao CSV.
- Experimento automatico de Belady.
- Testes automatizados e Valgrind.

## Limitacoes conhecidas

Estas limitacoes devem ser consideradas na defesa e em proximas evolucoes:

- O COW fisico funciona quando existe quadro livre para a copia, mas ainda nao
  resolve completamente o caso em que a memoria fisica esta cheia.
- A substituicao de paginas recusa vitimas com mais de um mapeamento COW. Esse
  comportamento evita corromper processos compartilhando um quadro, mas limita a
  integracao entre COW, substituicao e swap.
- O estresse do CLI valida consistencia matematica das metricas, mas nao valida
  preservacao byte a byte de todos os valores escritos.
- A validacao estrutural verifica o proprietario fisico principal do quadro, mas
  ainda pode ser ampliada para auditar todos os mapeamentos compartilhados pelo
  COW manager.
- O projeto nao implementa concorrencia real entre threads; o tema de memoria
  virtual e tratado como simulador sequencial orientado por traces.

## Diario de engenharia e IA

O arquivo `DIARIO.md` registra decisoes de projeto, problemas encontrados,
solucoes adotadas e uso de ferramentas de IA durante o desenvolvimento. Esse
diario deve ser mantido atualizado sempre que novas limitacoes forem corrigidas
ou novas decisoes tecnicas forem tomadas.
