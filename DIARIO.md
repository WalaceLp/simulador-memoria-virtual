# Diário de Engenharia

## 19/06

### Objetivos do dia

* Realizar a escolha do tema do trabalho interdisciplinar.
* Compreender os requisitos gerais do projeto.
* Iniciar o estudo sobre o funcionamento da memória virtual e da estrutura de dados exigida.

### Atividades realizadas

Durante a definição dos temas, o grupo ficou responsável pelo **Tema 3 – Tabela de Páginas Multinível com Trie de Tradução de Endereços**.

Após a escolha, foi realizada uma leitura completa do documento para identificar os requisitos obrigatórios do projeto, principalmente aqueles relacionados à estrutura de dados central, aos mecanismos de Sistemas Operacionais e ao teste de fogo exigido.

Também foi iniciada uma pesquisa sobre memória virtual, paginação, TLB (Translation Lookaside Buffer), page faults, algoritmos de substituição de páginas e funcionamento das tabelas de páginas multinível utilizadas em arquiteturas modernas, como o x86-64.

### Decisões de projeto

Nesta etapa foi decidido que o projeto será desenvolvido de forma incremental, priorizando inicialmente a correta modelagem da estrutura de dados (Trie da tabela de páginas), deixando a implementação dos mecanismos de memória virtual para as etapas seguintes.

Também foi decidido utilizar Git desde o início do desenvolvimento, registrando commits pequenos e frequentes para documentar a evolução do projeto.

### Estudos e pesquisas

Os principais conceitos estudados foram:

* funcionamento da memória virtual;
* tradução de endereços virtuais para físicos;
* organização de tabelas de páginas multinível;
* estrutura Trie (Radix Tree);
* funcionamento da TLB;
* page walk;
* page faults;
* algoritmos FIFO, LRU, Clock e Aging;
* conceito de swap e memória secundária;
* copy-on-write durante operações de fork.

Foi identificado que a tabela de páginas multinível pode ser representada naturalmente como uma Trie, onde cada nível corresponde a uma parte do endereço virtual utilizada durante o processo de tradução.


### Uso de IA

O ChatGPT e o Google Gemini foram utilizados como ferramenta de apoio para esclarecer conceitos relacionados à memória virtual, tabelas de páginas multinível, funcionamento da TLB e representação da tabela de páginas como uma Trie.

As respostas obtidas serviram como material complementar aos conteúdos estudados na disciplina e à documentação oficial do trabalho.

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
* Estruturar as pastas do projeto.



### Atividades realizadas

Foi criado o repositório do projeto no GitHub e realizada sua configuração inicial utilizando Git.

Em seguida, foi definida a estrutura de diretórios que será utilizada durante todo o desenvolvimento, separando o projeto em módulos independentes para facilitar a manutenção, os testes e a divisão das responsabilidades entre os integrantes da equipe.

Também foram criados os arquivos iniciais do projeto, incluindo README.md, DIARIO.md, Makefile e .gitignore, além das pastas destinadas ao código-fonte, arquivos de cabeçalho, testes, documentação, traces de memória, scripts de automação e resultados experimentais.

Após a organização inicial, foi realizado o primeiro commit do projeto contendo exclusivamente sua estrutura base.

### Decisões de projeto

Foi decidido adotar uma arquitetura modular, separando cada componente principal do simulador em arquivos independentes.

A organização inicial prevê módulos específicos para a tabela de páginas, TLB, memória física, algoritmos de substituição, swap, leitura de traces, gerenciamento de processos e coleta de métricas.

Essa separação permitirá desenvolver e testar cada componente individualmente antes de integrá-los ao simulador completo.

Também foi definido que os commits serão realizados de forma incremental, registrando pequenas evoluções do projeto em vez de grandes alterações acumuladas.

### Estudos e pesquisas

Foi realizada uma pesquisa sobre boas práticas para organização de projetos em linguagem C, estudando a separação entre arquivos de implementação (.c) e cabeçalhos (.h), além da organização tradicional de projetos utilizando diretórios específicos para código-fonte, testes, documentação e arquivos gerados durante a compilação.

Também foi estudada a estrutura necessária para atender aos requisitos do edital, especialmente em relação ao Makefile, testes automatizados e organização do repositório.

Problemas encontrados

Durante o processo de configuração inicial foi encontrada uma dificuldade na autenticação do GitHub utilizando chave SSH, impedindo o envio do primeiro commit ao repositório remoto.

### Soluções adotadas

Foi realizada a reconfiguração da autenticação do GitHub utilizando HTTPS, permitindo concluir corretamente o primeiro envio do projeto para o repositório remoto.

Também foi revisada toda a estrutura de diretórios antes do início da implementação para evitar reorganizações futuras que poderiam dificultar o histórico de commits.

### Uso de IA

O ChatGPT foi utilizado como ferramenta de apoio para discutir diferentes formas de organizar um projeto em linguagem C e avaliar qual estrutura seria mais adequada ao desenvolvimento do simulador de memória virtual.

As sugestões foram analisadas pela equipe e adaptadas às necessidades específicas do projeto antes de sua adoção.

### Próximos passos
* Implementar a estrutura inicial da tabela de páginas multinível.
* Definir as estruturas de dados básicas do simulador.
* Configurar a compilação automática pelo Makefile.
* Iniciar os primeiros testes unitários.