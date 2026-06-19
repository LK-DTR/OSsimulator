# Simulador de SO em C++ com Qt

## Visão Geral

Este projeto é uma aplicação em C++ com uma interface gráfica de usuário construída usando Qt e QML. Possui um motor de simulação (`simulador.hpp`) e depende de entradas de dados externos via arquivos CSV.

## Estrutura do Projeto

- `main.cpp`: O ponto de entrada da aplicação C++. Geralmente inicializa o motor da aplicação Qt, registra classes de backend do C++ no QML e carrega a interface frontend.
- `Main.qml`: O arquivo QML principal que define a interface gráfica do usuário.
- `simulador.hpp`: Backend Qt (`QObject`). Lê e faz o parse do CSV, chama o núcleo de simulação e expõe o relatório e a linha do tempo ao QML.
- `core/`: Núcleo de simulação em C++ puro (sem Qt), dividido em módulos por responsabilidade:
  - `core/tipos.hpp`: estruturas e enums compartilhados (`Proc`, `Result`, `Sched`, `Repl`, etc.).
  - `core/scheduler.hpp`: escalonadores RR, SJF preemptivo e Prioridade preemptiva; gera a linha do tempo e as métricas.
  - `core/memory.hpp`: paginação e substituição de páginas (FIFO, LRU, Ótimo); string de referência e contagem de page faults.
  - `core/simulacao.hpp`: coordena os módulos e executa a simulação completa (`simular`).
- `example.csv`: Um arquivo de valores separados por vírgula (CSV) de exemplo, contendo dados usados ou processados pela aplicação.
- `CMakeLists.txt`: O arquivo de configuração de compilação do CMake que define os alvos (targets), vincula as bibliotecas do Qt e gerencia a compilação.
- `.gitignore`: Especifica arquivos intencionalmente não rastreados (como diretórios de compilação e binários) que o Git deve ignorar.

## Modelo de Simulação

- **Escalonamento** (passo de tempo = 1 unidade): RR usa fila FIFO com o quantum
  definido na interface; SJF preemptivo (SRTF) e Prioridade preemptiva reavaliam o
  processo a cada unidade de tempo. Na Prioridade, **menor valor = maior prioridade**.
- **Memória**: cada página/frame ocupa `PAGE_SIZE_MB` (10 MB, em `core.hpp`). O número
  de frames físicos é `Memória Física / PAGE_SIZE_MB`. A cada unidade de tempo o processo
  em execução referencia uma de suas páginas (coluna `paginas` do CSV ou derivadas da
  memória necessária), gerando a string de referência usada na substituição de páginas.
  Para observar page faults e substituição, defina uma memória física pequena.
- **Memória virtual**: define a capacidade total em páginas (`Memória Virtual / PAGE_SIZE_MB`).
  Se o total de páginas distintas exigidas pelos processos ultrapassar essa capacidade, o
  relatório sinaliza estouro de memória virtual.
- **Tabela de páginas**: ao final, o relatório mostra o estado de cada frame físico
  (qual processo/página está residente ou se o frame está livre).

## Formato do CSV

Aceita o formato do enunciado — `chegada,burst,prioridade,memoria` (com ou sem
cabeçalho; o PID é atribuído automaticamente) — e também formatos estendidos com as
colunas opcionais `pid` e `paginas` (ver `example.csv`). Com cabeçalho, as colunas são
identificadas pelo nome (inclusive com acentos); sem cabeçalho, pela quantidade de colunas.

## Dependências

Para compilar e executar este projeto, você precisará ter o seguinte instalado em seu sistema:

- **Compilador C++**: Um compilador C++ padrão (ex: GCC, Clang ou MSVC) com suporte a C++11/17 ou superior.
- **CMake**: Versão 3.16 ou mais recente (verifique o seu `CMakeLists.txt` específico para saber a versão mínima exigida).
- **Qt Framework**: Qt 5 ou Qt 6 (especificamente os módulos Core, Gui, Qml e Quick).

### Instalando Dependências (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev
```

## Instruções de Compilação

1. Abra um terminal e navegue até a raiz do diretório do projeto.
2. Crie um novo diretório chamado `build` e entre nele:
    ```bash
    mkdir build
    cd build
    ```
3. Gere os arquivos de compilação usando o CMake:
    ```bash
    cmake ..
    ```
4. Compile o projeto:
    ```bash
    cmake --build .
    ```

## Executando a Aplicação

Assim que o processo de compilação for concluído com sucesso, um executável será gerado dentro do diretório `build`. Você pode executá-lo diretamente pelo terminal.

```bash
./simulador_so
```
