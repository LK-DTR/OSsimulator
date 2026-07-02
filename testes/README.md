# Bateria de testes da interface

Conjunto de arquivos CSV para validar o simulador pela interface — casos
corretos (assertivos) e casos de erro/entrada inválida. Em cada um, selecione o
arquivo em **Procurar**, ajuste as opções indicadas e clique em **Iniciar
Simulação**.

## Casos assertivos (devem produzir resultado correto)

| Arquivo | Opções | Resultado esperado |
|---|---|---|
| `../example.csv` | SJF Preemptivo · Física 30 · Virtual 400 · FIFO | Gantt P1[0,5) P2[5,8) P3[8,15); espera média 2,33; **7 page faults** |
| `teste_sjf.csv` | SJF Preemptivo | Conclusões 18/5/26/10; resposta 4,50; espera 6,75 |
| `teste_rr.csv` | Round-Robin · quantum 2 | Resposta média 1,00; espera média 5,75 |
| `teste_memoria.csv` | Física 30 · FIFO / LRU / Ótimo | **15 / 12 / 9** page faults |
| `cabecalho_composto.csv` | SJF Preemptivo | Igual ao `example` sem páginas: conclusões 5/8/15 — valida cabeçalhos como "Tempo de Chegada", "Memória Necessária" |

## Casos de erro / entrada inválida (devem ser tratados sem travar)

| Arquivo | O que testa | Comportamento esperado |
|---|---|---|
| `erros/vazio.csv` | Arquivo sem conteúdo | Mensagem: "Arquivo CSV vazio ou sem linhas válidas" |
| `erros/so_comentarios.csv` | Apenas comentários e linhas em branco | Mesma mensagem de arquivo vazio |
| `erros/burst_invalido.csv` | Linhas com burst 0 e negativo misturadas a uma válida | Ignora as inválidas; simula só o processo válido (P3) |
| `erros/todos_invalidos.csv` | Todos os processos com burst <= 0 | Mensagem: "Nenhum processo válido encontrado" |
| `erros/valores_texto.csv` | Campos não numéricos ("zero", "cinco") | Usa valores padrão (chegada 0, burst 1) e simula mesmo assim |

## Observações

- O simulador ignora linhas em branco e comentários (iniciados por `#` ou `//`).
- Campos ausentes ou não numéricos assumem valores padrão por campo.
- Processos sem tempo de CPU (burst <= 0) são descartados na leitura, evitando
  simulações inválidas.
- As mensagens de erro aparecem na própria área de relatório da interface.
