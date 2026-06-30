# Diário de Engenharia

## 19/06

### Objetivos do dia

* Realizar a escolha do tema do trabalho interdisciplinar.
* Compreender os requisitos gerais do projeto.
* Iniciar o estudo sobre o funcionamento da memória virtual e da estrutura de dados exigida.

### Atividades realizadas

Durante a definição dos temas, o grupo ficou responsável pelo **Tema 3 – Tabela de Páginas Multinível com Trie de Tradução de Endereços**.

Após a escolha, foi realizada uma leitura completa do documento do trabalho para identificar os requisitos obrigatórios do projeto, principalmente aqueles relacionados à estrutura de dados central, aos mecanismos de Sistemas Operacionais e aos testes de fogo exigidos.

Também foi iniciada uma pesquisa sobre memória virtual, paginação, TLB (*Translation Lookaside Buffer*), page faults, algoritmos de substituição de páginas e funcionamento das tabelas de páginas multinível utilizadas em arquiteturas modernas, como o x86-64.

### Decisões de projeto

Nesta etapa foi decidido que o projeto seria desenvolvido de forma incremental, priorizando inicialmente a correta modelagem da estrutura de dados central, representada pela Trie da tabela de páginas.

Os mecanismos de memória virtual seriam adicionados posteriormente, após a validação das operações básicas da estrutura.

Também foi decidido utilizar Git desde o início do desenvolvimento, registrando commits pequenos e frequentes para documentar a evolução do projeto.

### Estudos e pesquisas

Os principais conceitos estudados foram:

* funcionamento da memória virtual;
* tradução de endereços virtuais para físicos;
* organização de tabelas de páginas multinível;
* estrutura Trie, também conhecida como Radix Tree;
* funcionamento da TLB;
* page walk;
* page faults;
* algoritmos FIFO, LRU, Clock e Aging;
* swap e memória secundária;
* compartilhamento de páginas;
* copy-on-write durante operações de fork.

Foi identificado que a tabela de páginas multinível pode ser representada naturalmente como uma Trie, na qual cada nível corresponde a uma parte do endereço virtual utilizada durante o processo de tradução.

### Problemas encontrados

Nesta etapa inicial, a principal dificuldade foi compreender como relacionar a estrutura de dados estudada em Estruturas de Dados com os mecanismos de memória virtual estudados em Sistemas Operacionais.

### Soluções adotadas

Os requisitos foram separados em grupos menores:

* decomposição dos endereços virtuais;
* implementação da Trie;
* gerenciamento de processos;
* memória física;
* substituição de páginas;
* TLB;
* swap;
* testes e experimentos.

Essa divisão tornou o desenvolvimento mais organizado e permitiu planejar uma implementação incremental.

### Uso de IA

O ChatGPT e o Google Gemini foram utilizados como ferramentas de apoio para esclarecer conceitos relacionados à memória virtual, tabelas de páginas multinível, funcionamento da TLB e representação da tabela de páginas como uma Trie.

As respostas obtidas serviram como material complementar aos conteúdos estudados na disciplina e ao documento oficial do trabalho.

Nenhum código foi gerado nesta etapa.

### Próximos passos

* Organizar o repositório Git.
* Definir a arquitetura geral do projeto.
* Estruturar as pastas do projeto.
* Definir os módulos principais do simulador.
* Iniciar a implementação da estrutura básica da Trie utilizada pela tabela de páginas.


## 22/06

### Objetivos do dia

* Organizar o repositório Git.
* Definir a arquitetura geral do projeto.
* Estruturar as pastas e arquivos iniciais.
* Preparar o ambiente de compilação e testes.

### Atividades realizadas

Foi criado o repositório do projeto no GitHub e realizada sua configuração inicial utilizando Git.

Em seguida, foi definida a estrutura de diretórios que seria utilizada durante o desenvolvimento, separando o projeto em módulos independentes para facilitar a manutenção, os testes e a divisão de responsabilidades.

Também foram criados os arquivos iniciais do projeto, incluindo:

* `README.md`;
* `DIARIO.md`;
* `Makefile`;
* `.gitignore`;
* diretório de código-fonte;
* diretório de cabeçalhos;
* diretório de testes;
* diretório de documentação;
* diretório de traces;
* diretório de scripts;
* diretório de resultados experimentais.

Após a organização inicial, foi realizado o primeiro commit contendo exclusivamente a estrutura base do projeto.

### Decisões de projeto

Foi decidido adotar uma arquitetura modular, separando cada componente principal do simulador em arquivos independentes.

A organização inicial previu módulos específicos para:

* decomposição de endereços;
* tabela de páginas;
* processos;
* memória física;
* algoritmos de substituição;
* TLB;
* swap;
* leitura de traces;
* geração de traces;
* interface de linha de comando;
* coleta e exportação de métricas.

Também foi definido que os commits seriam realizados de forma incremental, registrando pequenas evoluções em vez de grandes alterações acumuladas.

### Estudos e pesquisas

Foi realizada uma pesquisa sobre boas práticas para organização de projetos em linguagem C, estudando:

* separação entre arquivos `.c` e `.h`;
* proteção de cabeçalhos;
* organização do Makefile;
* compilação com C11;
* utilização das opções `-Wall`, `-Wextra` e `-Werror`;
* criação de testes independentes;
* organização de artefatos gerados durante a compilação.

Também foi revisada a estrutura necessária para atender aos requisitos do trabalho, especialmente em relação ao Makefile, aos testes automatizados e à organização do repositório.

### Problemas encontrados

Durante a configuração inicial foi encontrada uma dificuldade na autenticação do GitHub utilizando chave SSH, impedindo o envio do primeiro commit ao repositório remoto.

### Soluções adotadas

Foi realizada a reconfiguração da autenticação do GitHub utilizando HTTPS, permitindo concluir corretamente o envio inicial do projeto.

A estrutura de diretórios também foi revisada antes do início da implementação para evitar reorganizações futuras que pudessem prejudicar o histórico de commits.

### Uso de IA

O ChatGPT foi utilizado como ferramenta de apoio para discutir diferentes formas de organizar um projeto em linguagem C e avaliar qual estrutura seria mais adequada ao simulador.

As sugestões foram analisadas e adaptadas às necessidades do projeto antes de serem utilizadas.

### Próximos passos

* Implementar a decomposição dos endereços virtuais.
* Definir as constantes relacionadas ao tamanho das páginas.
* Implementar a estrutura inicial da tabela de páginas multinível.
* Criar os primeiros testes unitários.
* Configurar os primeiros alvos do Makefile.


## 23/06

### Objetivos do dia

* Implementar a decomposição dos endereços virtuais.
* Definir o formato da tabela de páginas de quatro níveis.
* Criar a estrutura inicial da Trie.
* Iniciar os testes unitários da estrutura central.

### Atividades realizadas

Foi implementado o módulo responsável pela decomposição de endereços virtuais.

O endereço foi organizado considerando páginas de 4 KiB e uma estrutura semelhante à utilizada no x86-64:

* quatro níveis de tabela;
* nove bits de índice por nível;
* doze bits de deslocamento dentro da página.

Foram criadas funções para obter:

* índice correspondente a cada nível;
* deslocamento dentro da página;
* número da página virtual.

Também foi criada a estrutura inicial da tabela de páginas, formada por nós intermediários e entradas folha.

Os primeiros testes foram adicionados para verificar a decomposição de diferentes endereços e os valores dos índices calculados.

### Decisões de projeto

Foi decidido utilizar uma Trie de quatro níveis com criação dinâmica dos nós.

Cada nível utiliza 512 possibilidades de entrada, correspondentes aos nove bits extraídos do endereço virtual.

As páginas possuem tamanho fixo de 4096 bytes.

Também foi decidido manter os detalhes internos da Trie encapsulados no módulo da tabela de páginas, expondo apenas as operações necessárias por meio do cabeçalho público.

### Estudos e pesquisas

Foram estudados:

* divisão de endereços virtuais em campos;
* uso de máscaras de bits;
* deslocamentos binários;
* cálculo do número da página virtual;
* estrutura das tabelas de páginas x86-64;
* diferença entre nós intermediários e entradas folha.

### Problemas encontrados

Foi necessário garantir que os deslocamentos e máscaras fossem aplicados corretamente em valores de 64 bits.

Também foi necessário diferenciar o deslocamento dentro da página dos índices utilizados nos quatro níveis da Trie.

### Soluções adotadas

Foram criadas constantes específicas para:

* quantidade de níveis;
* quantidade de bits por índice;
* quantidade de bits do deslocamento;
* tamanho da página;
* número de entradas por nível.

Os testes utilizaram endereços conhecidos para confirmar manualmente os índices e o deslocamento retornados.

### Uso de IA

O ChatGPT foi utilizado para revisar a lógica de decomposição binária dos endereços e sugerir casos de teste.

O código e os resultados foram revisados antes de serem incorporados ao projeto.

### Próximos passos

* Implementar as operações de mapeamento.
* Implementar a busca por traduções.
* Criar os níveis da Trie apenas quando necessários.
* Adicionar testes de criação sob demanda.


## 24/06

### Objetivos do dia

* Implementar o mapeamento de páginas.
* Implementar a busca por traduções.
* Criar os subníveis da Trie sob demanda.
* Implementar desmapeamento e liberação de subárvores vazias.

### Atividades realizadas

Foi implementada a operação de mapeamento entre um endereço virtual e um quadro físico.

Durante o mapeamento, os níveis intermediários da Trie são criados somente quando o caminho correspondente ainda não existe.

Também foi implementada a busca por traduções, percorrendo os quatro níveis até localizar a entrada folha correspondente à página virtual.

Em seguida, foi criada a operação de desmapeamento. Após remover uma entrada folha, o algoritmo verifica os níveis superiores e libera os nós que ficaram vazios.

Foram adicionados testes para:

* mapear uma página;
* localizar um mapeamento existente;
* retornar ausência para uma página não mapeada;
* mapear páginas com prefixos compartilhados;
* substituir um mapeamento;
* desmapear uma página;
* preservar outras páginas do mesmo caminho;
* liberar subárvores que ficaram vazias.

### Decisões de projeto

Foi decidido não alocar antecipadamente todos os níveis possíveis da tabela de páginas.

A criação sob demanda reduz o consumo de memória e representa melhor o comportamento das tabelas multinível reais.

A remoção também foi projetada para realizar a poda da Trie, liberando nós intermediários sem filhos.

### Estudos e pesquisas

Foram estudados:

* criação preguiçosa de estruturas;
* percursos em árvores multinível;
* poda de árvores;
* gerenciamento manual de memória em C;
* riscos de vazamentos e acessos após liberação.

### Problemas encontrados

A principal dificuldade foi garantir que a remoção de uma página não liberasse nós que ainda eram utilizados por outros mapeamentos.

Também foi necessário garantir que todos os caminhos de erro liberassem corretamente os recursos já alocados.

### Soluções adotadas

O caminho percorrido durante o desmapeamento foi armazenado temporariamente.

A verificação dos nós vazios foi realizada de baixo para cima, interrompendo a poda assim que um nível ainda possuía outra entrada.

Foram criados testes com páginas que compartilhavam os níveis iniciais da Trie.

### Uso de IA

O ChatGPT foi utilizado como apoio na revisão do algoritmo de poda e na identificação de casos de teste relacionados ao compartilhamento de prefixos.

As sugestões foram adaptadas à estrutura implementada no projeto.

### Próximos passos

* Criar o módulo de processos.
* Associar cada processo a uma tabela de páginas.
* Implementar a operação de fork.
* Adicionar compartilhamento estrutural e copy-on-write na Trie.


## 25/06

### Objetivos do dia

* Implementar o gerenciamento de processos.
* Associar processos às suas tabelas de páginas.
* Implementar o fork.
* Implementar compartilhamento estrutural e copy-on-write da Trie.

### Atividades realizadas

Foi criado o módulo de processos, responsável por armazenar:

* PID;
* tabela de páginas associada;
* ciclo de criação e destruição.

Também foi implementada a operação de fork.

Inicialmente, pai e filho compartilham a raiz e as subárvores existentes da tabela de páginas.

Foram adicionados contadores de referência aos nós e às entradas folha para controlar o compartilhamento.

Quando um dos processos modifica uma parte compartilhada da tabela, somente o caminho necessário é copiado. Dessa forma, as demais partes continuam compartilhadas.

Foram criados testes para:

* criação e destruição de processos;
* obtenção do PID;
* acesso à tabela associada;
* fork;
* compartilhamento inicial da raiz;
* preservação do processo pai após alterações no filho;
* preservação do filho após alterações no pai;
* liberação correta das estruturas compartilhadas.

### Decisões de projeto

Foi decidido utilizar copy-on-write estrutural na Trie.

A estrutura somente é copiada quando uma operação de mapeamento ou desmapeamento precisa alterar um caminho compartilhado.

Essa abordagem evita copiar toda a tabela de páginas durante o fork.

O copy-on-write implementado nesta etapa foi restrito à estrutura da tabela. A cópia do conteúdo físico das páginas seria integrada posteriormente.

### Estudos e pesquisas

Foram estudados:

* fork;
* compartilhamento de tabelas de páginas;
* contadores de referência;
* copy-on-write;
* cópia superficial de caminhos;
* destruição segura de estruturas compartilhadas.

### Problemas encontrados

Foi necessário impedir que a destruição de um processo liberasse nós ainda utilizados por outro processo.

Também foi necessário evitar que uma modificação realizada pelo filho alterasse diretamente a tabela compartilhada pelo pai.

### Soluções adotadas

Cada nó e entrada compartilhada passou a possuir contador de referências.

A liberação somente ocorre quando o contador chega a zero.

Antes de modificar um caminho compartilhado, o simulador cria uma cópia dos níveis necessários e atualiza apenas a tabela do processo responsável pela alteração.

### Uso de IA

O ChatGPT foi utilizado para discutir alternativas de implementação do copy-on-write e revisar a utilização dos contadores de referência.

Os testes foram ampliados para verificar a independência entre pai e filho após modificações.

### Próximos passos

* Implementar a memória física.
* Representar os quadros e seus metadados.
* Implementar os algoritmos de substituição.
* Integrar os processos à memória física.


## 26/06

### Objetivos do dia

* Implementar o módulo de memória física.
* Representar quadros ocupados e livres.
* Implementar FIFO, LRU, Clock e Aging.
* Criar uma interface comum para as políticas de substituição.

### Atividades realizadas

Foi criado o módulo de memória física.

Cada quadro passou a armazenar informações como:

* estado de ocupação;
* PID proprietário;
* processo proprietário;
* página virtual;
* bit de modificação;
* bit de referência.

Foram implementadas operações para:

* alocar um quadro livre;
* substituir o conteúdo de um quadro;
* liberar um quadro;
* marcar acessos de leitura e escrita;
* consultar os metadados de um quadro.

Também foi criada uma interface comum para os algoritmos de substituição.

Foram implementadas as políticas:

* FIFO;
* LRU;
* Clock;
* Aging.

Cada política possui operações comuns para registrar carregamentos, acessos, remoções, passagem do tempo e seleção de vítimas.

Foram adicionados testes específicos para cada algoritmo.

### Decisões de projeto

Foi decidido separar os algoritmos de substituição da memória virtual.

A memória virtual utiliza uma interface comum e não precisa conhecer os detalhes internos de cada política.

O FIFO utiliza uma fila de carregamento.

O LRU utiliza instantes lógicos de acesso.

O Clock utiliza o bit de referência e um ponteiro circular.

O Aging utiliza contadores de envelhecimento atualizados ao longo dos acessos.

### Estudos e pesquisas

Foram revisados:

* funcionamento dos quadros físicos;
* bits de referência e modificação;
* políticas locais de substituição;
* anomalia de Belady;
* diferença entre algoritmos exatos e aproximações;
* envelhecimento de páginas.

### Problemas encontrados

Foi necessário garantir que as quatro políticas utilizassem a mesma interface sem perder seus comportamentos específicos.

Também foi necessário lidar com quadros que ainda não estavam ocupados e impedir que fossem selecionados como vítimas.

### Soluções adotadas

Foi criada uma estrutura abstrata de política de substituição.

Cada implementação mantém seu próprio estado interno, mas responde ao mesmo conjunto de operações.

Os testes verificaram tanto a seleção das vítimas quanto a atualização interna de cada algoritmo.

### Uso de IA

O ChatGPT foi utilizado como apoio para revisar as diferenças entre FIFO, LRU, Clock e Aging e para sugerir sequências de acesso úteis nos testes.

As implementações foram validadas individualmente antes da integração.

### Próximos passos

* Integrar a memória física às tabelas de páginas.
* Implementar o tratamento de page faults.
* Registrar leituras, escritas e substituições.
* Implementar a TLB.


## 27/06

### Objetivos do dia

* Integrar processos, tabelas de páginas e memória física.
* Implementar page faults.
* Integrar os algoritmos de substituição.
* Implementar a TLB e as métricas de page walk.

### Atividades realizadas

Foi criado o módulo principal de memória virtual.

O fluxo de acesso passou a:

1. identificar o processo;
2. calcular a página virtual;
3. consultar a TLB;
4. consultar a tabela de páginas em caso de miss;
5. tratar o page fault quando a página está ausente;
6. alocar ou substituir um quadro;
7. atualizar a tabela de páginas;
8. registrar o acesso na política de substituição.

Foram implementadas estatísticas para:

* acessos totais;
* leituras;
* escritas;
* page faults;
* substituições;
* vítimas sujas.

Também foi implementada uma TLB totalmente associativa, identificando traduções pela combinação de PID e página virtual.

A TLB utiliza LRU internamente para substituir suas próprias entradas.

Foram adicionadas operações para:

* consultar uma tradução;
* inserir ou atualizar uma entrada;
* invalidar uma página;
* invalidar todas as entradas de um processo;
* limpar completamente a TLB.

A busca na tabela de páginas também passou a registrar quantos níveis foram percorridos.

Foram adicionadas métricas de:

* hits da TLB;
* misses da TLB;
* taxa de acerto;
* page walks;
* total de níveis visitados;
* média de níveis por caminhamento.

### Decisões de projeto

Foi decidido consultar a TLB antes da Trie.

A chave da TLB inclui o PID para impedir conflitos entre processos que utilizem o mesmo endereço virtual.

Quando um quadro é reutilizado, a tradução antiga é invalidada antes da inserção da nova tradução.

Um miss da TLB não é tratado automaticamente como page fault, pois a página pode continuar presente na tabela e na memória física.

### Estudos e pesquisas

Foram revisados:

* funcionamento da TLB;
* diferença entre TLB miss e page fault;
* tradução por PID e página;
* page walks;
* invalidação de traduções;
* métricas de desempenho da tradução.

### Problemas encontrados

Foi necessário impedir que a TLB mantivesse traduções obsoletas após a substituição de uma página.

Também foi necessário contar corretamente os níveis percorridos nos casos em que a busca terminava antes do quarto nível.

### Soluções adotadas

As entradas antigas passaram a ser invalidadas antes da reutilização dos quadros.

Foi criada uma variante da operação de busca que retorna a quantidade de níveis visitados.

Foram adicionados testes para hits, misses, invalidação, diferenciação de processos e caminhamentos completos pela Trie.

### Uso de IA

O ChatGPT foi utilizado para revisar o fluxo integrado de acesso e para sugerir testes envolvendo TLB miss sem page fault e invalidação após substituição.

A integração foi validada com testes unitários e testes do módulo de memória virtual.

### Próximos passos

* Implementar o conteúdo real dos quadros.
* Criar o arquivo binário de swap.
* Implementar swap-out e swap-in.
* Implementar a leitura de traces.


## 28/06

### Objetivos do dia

* Implementar o swap em arquivo binário.
* Armazenar conteúdo real nos quadros físicos.
* Preservar páginas sujas durante substituições.
* Implementar a leitura e reprodução de traces.

### Atividades realizadas

A memória física foi ampliada para armazenar 4096 bytes de conteúdo por quadro.

Foram adicionadas operações para:

* ler uma página completa;
* escrever uma página completa;
* ler um byte;
* escrever um byte.

Também foi criado o módulo de swap.

O arquivo de swap foi dividido em slots de 4 KiB. Cada slot é identificado pela combinação de PID e página virtual.

Quando uma página suja é escolhida como vítima, seu conteúdo é copiado para o arquivo de swap antes da reutilização do quadro.

Quando uma página armazenada no swap é acessada novamente, o simulador realiza o swap-in, restaura os dados no quadro e libera o slot utilizado.

Também foi criado o leitor de traces.

O formato simplificado aceita operações como:

```text
R 0x1000
W 0x2000

Também foram reconhecidas as operações do Valgrind/Lackey:

* `I` para busca de instrução;
* `L` para leitura;
* `S` para escrita;
* `M` para leitura seguida de escrita.

Foram adicionados testes para verificar a preservação dos dados após ciclos de swap-out e swap-in.

### Decisões de projeto

Foi decidido armazenar os dados dos quadros em um bloco contínuo de memória.

O swap possui uma quantidade fixa de slots definida durante sua criação.

Somente páginas sujas são gravadas no arquivo, evitando operações desnecessárias para páginas limpas.

Após o swap-in, o slot correspondente é liberado para reutilização.

O leitor de traces foi mantido separado da memória virtual, sendo responsável apenas por interpretar as linhas e executar os acessos correspondentes.

### Estudos e pesquisas

Foram estudados:

* memória secundária;
* swap-out;
* swap-in;
* páginas limpas e sujas;
* organização de arquivos binários;
* acesso por deslocamentos;
* formato de saída do Valgrind/Lackey;
* acessos que atravessam limites de página.

### Problemas encontrados

Foi necessário garantir que o conteúdo de uma página fosse salvo antes que seu quadro fosse reutilizado.

Também foi necessário impedir que páginas de processos diferentes, ocupando o mesmo endereço virtual, fossem confundidas no swap.

Outro problema foi tratar a operação `M` do Lackey como dois acessos distintos.

### Soluções adotadas

O swap-out passou a ocorrer antes da atualização do quadro vítima.

Os slots são identificados pela combinação de PID e número da página virtual.

A operação `M` gera uma leitura imediata e uma escrita pendente.

O leitor também calcula todas as páginas afetadas quando um acesso atravessa o limite entre duas páginas.

### Uso de IA

O ChatGPT foi utilizado para revisar a organização do arquivo binário, os casos de erro e a interpretação das operações emitidas pelo Lackey.

Os testes incluíram leitura, escrita, detecção de swap cheio, remoção de slots e preservação do conteúdo das páginas.

### Próximos passos

* Criar a interface de linha de comando.
* Permitir a configuração de quadros, TLB, política e swap.
* Exportar métricas para CSV.
* Criar traces sintéticos para os experimentos.


## 29/06

### Objetivos do dia

* Criar a interface final de linha de comando.
* Exportar métricas em CSV.
* Implementar o gerador de traces.
* Automatizar o experimento da anomalia de Belady.

### Atividades realizadas

Foi criado o módulo de interface de linha de comando.

O executável principal passou a aceitar parâmetros para:

* caminho do trace;
* quantidade de quadros;
* política de substituição;
* tamanho da TLB;
* arquivo de swap;
* número de slots;
* PID simulado;
* arquivo CSV de saída.

Também foi criado o módulo de exportação de métricas em CSV.

Cada execução pode registrar:

* configuração utilizada;
* acessos processados;
* leituras e escritas;
* page faults;
* substituições;
* vítimas sujas;
* hits e misses da TLB;
* taxa de acerto;
* page walks;
* média de níveis;
* leituras e escritas no swap.

O arquivo pode ser sobrescrito ou receber novas linhas.

Também foi criado o executável `tracegen`, responsável por gerar traces sintéticos.

O gerador permite configurar:

* padrão sequencial ou aleatório;
* quantidade de páginas;
* quantidade de acessos;
* percentual de escritas;
* semente pseudoaleatória;
* endereço base.

Por fim, foi criado o experimento automatizado da anomalia de Belady com a sequência:

```text
1 2 3 4 1 2 5 1 2 3 4 5