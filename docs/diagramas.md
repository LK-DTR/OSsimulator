# Fluxogramas do Simulador de SO

Diagramas de apoio à apresentação. Renderizam automaticamente no GitHub e no
VS Code. As imagens correspondentes (PNG) estão em `docs/img/`.

> Para a apresentação, os principais são: **1. Fluxo geral**, **2. Arquitetura**,
> **4. Escalonamento** e **5. Memória virtual**. Os diagramas 3 (CSV) e 6
> (Relatório) servem de detalhamento, se houver tempo.

## 1. Fluxograma geral do sistema

![Fluxo geral](img/01_fluxo_geral.png)

```mermaid
flowchart TD
    A["Usuário abre o simulador"] --> B["Seleciona arquivo CSV"]
    B --> C["Define memória física e virtual"]
    C --> D["Escolhe escalonamento"]
    D --> E["Escolhe política de substituição"]
    E --> F["Clica em Iniciar Simulação"]
    F --> G["Backend lê e valida os dados"]
    G --> H["Núcleo executa escalonamento"]
    H --> I["Calcula métricas (tempos médios)"]
    I --> J["Núcleo simula memória virtual"]
    J --> K["Gera Gantt e relatório"]
    K --> L["Interface exibe resultados"]
```

## 2. Arquitetura do código

![Arquitetura](img/02_arquitetura.png)

```mermaid
flowchart TD
    A["main.cpp"] --> B["Main.qml"]
    A --> C["Simulador backend"]

    B -->|chama iniciarSimulacao| C

    C --> D["core/simulacao.hpp"]
    D --> E["core/scheduler.hpp"]
    D --> F["core/memory.hpp"]
    D --> G["core/tipos.hpp"]
    E --> G
    F --> G

    E --> H["Gantt e métricas"]
    F --> I["Page faults e tabela de páginas"]

    H --> C
    I --> C
    C -->|relatorio e gantt| B
```

## 3. Leitura do CSV

![Leitura do CSV](img/03_csv.png)

```mermaid
flowchart TD
    A["Abrir arquivo CSV"] --> B{"Arquivo abriu?"}
    B -->|Não| C["Exibir erro / encerrar"]
    B -->|Sim| D["Ler linhas"]
    D --> E["Ignorar linhas vazias ou comentários"]
    E --> F{"Tem cabeçalho?"}
    F -->|Sim| G["Mapear colunas pelo nome"]
    F -->|Não| H["Mapear colunas pela quantidade"]
    G --> I["Criar processos"]
    H --> I
    I --> J["Converter para core::Proc"]
    J --> K["Enviar para simulação"]
```

## 4. Escalonamento (RR, SJF-P e Prioridade-P)

![Escalonamento](img/04_escalonamento.png)

A seleção é reavaliada a cada unidade de tempo — é isso que torna SJF e
Prioridade preemptivos.

```mermaid
flowchart TD
    A["Início do escalonamento"] --> B["tempo = 0"]
    B --> C{"Todos processos terminaram?"}
    C -->|Sim| D["Gerar métricas e execTick"]
    C -->|Não| E["Verificar processos disponíveis"]

    E --> F{"Política escolhida"}
    F -->|Round-Robin| G["Usar fila e quantum"]
    F -->|SJF Preemptivo| H["Escolher menor tempo restante"]
    F -->|Prioridade Preemptiva| I["Escolher maior prioridade"]

    G --> J{"Há processo selecionado?"}
    H --> J
    I --> J

    J -->|Não| K["CPU ociosa"]
    J -->|Sim| L["Executar por 1 unidade de tempo"]

    K --> M["tempo++"]
    L --> M
    M --> C
```

## 5. Memória virtual (page fault e substituição)

![Memória virtual](img/05_memoria.png)

O escalonamento roda antes e gera a string de referência completa; por isso o
algoritmo Ótimo consegue "olhar o futuro".

```mermaid
flowchart TD
    A["Receber sequência de referências"] --> B["Calcular número de frames físicos"]
    B --> C["Calcular páginas virtuais disponíveis"]
    C --> D["Para cada referência de página"]
    D --> E{"Página está na memória física?"}
    E -->|Sim| F["Page hit"]
    E -->|Não| G["Page fault"]

    G --> H{"Há frame livre?"}
    H -->|Sim| I["Carregar página no frame"]
    H -->|Não| J{"Política de substituição"}

    J -->|FIFO| K["Remover página mais antiga"]
    J -->|LRU| L["Remover página menos usada recentemente"]
    J -->|Ótimo| M["Remover página usada mais tarde no futuro"]

    K --> N["Carregar nova página"]
    L --> N
    M --> N
    I --> O["Próxima referência"]
    N --> O
    F --> O
    O --> D
```

## 6. Geração do relatório

![Relatório](img/06_relatorio.png)

```mermaid
flowchart TD
    A["Receber Result do núcleo"] --> B["Criar dados do Gantt"]
    B --> C["Montar configuração"]
    C --> D["Montar métricas por processo"]
    D --> E["Ler médias do Result"]
    E --> F["Adicionar total de referências"]
    F --> G["Adicionar total de page faults"]
    G --> H["Adicionar tabela final de páginas"]
    H --> I["Emitir resultadoPronto"]
    I --> J["QML atualiza tela"]
```
